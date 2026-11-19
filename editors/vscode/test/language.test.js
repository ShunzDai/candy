'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { before, test } = require('node:test');
const { Registry, INITIAL } = require('vscode-textmate');
const { loadWASM, OnigScanner, OnigString } = require('vscode-oniguruma');

const root = path.resolve(__dirname, '..');
const json = file => JSON.parse(fs.readFileSync(path.join(root, file), 'utf8'));
let grammar;

before(async () => {
  await loadWASM(fs.readFileSync(require.resolve('vscode-oniguruma/release/onig.wasm')));
  const registry = new Registry({
    onigLib: Promise.resolve({
      createOnigScanner: patterns => new OnigScanner(patterns),
      createOnigString: text => new OnigString(text)
    }),
    loadGrammar: async scope => scope === 'source.candy'
      ? json('syntaxes/candy.tmLanguage.json')
      : null
  });
  grammar = await registry.loadGrammar('source.candy');
  assert.ok(grammar);
});

function tokens(line, state = INITIAL) {
  return grammar.tokenizeLine(line, state);
}

function assertScope(line, text, scope, state = INITIAL) {
  const result = tokens(line, state);
  const offset = line.indexOf(text);
  assert.ok(offset >= 0);
  const matching = result.tokens.filter(t => t.startIndex < offset + text.length && t.endIndex > offset);
  assert.ok(matching.length > 0 && matching.every(t => t.scopes.includes(scope)),
    `${JSON.stringify(text)} should have ${scope}: ${JSON.stringify(result.tokens)}`);
  return result.ruleStack;
}

// Representative script used by the tokenizer tests (kept inline so the
// extension tests do not depend on files outside the extension directory).
const SAMPLE = [
  '# 数独求解（回溯）',
  'def solve(grid)',
  '  if (solved(grid))',
  '    return true',
  '  end',
  '  for (cell : grid)',
  '    while (!cell)',
  '      print("try", cell)',
  '    end',
  '  end',
  '  return false',
  'end'
].join('\n');

test('highlights definitions, calls, builtins, literals and all lexer keywords', () => {
  assertScope('def solve(g, r)', 'def', 'keyword.control.candy');
  assertScope('def solve(g, r)', 'solve', 'entity.name.function.candy');
  assertScope('solve(grid, 0)', 'solve', 'entity.name.function.call.candy');
  for (const builtin of ['print', 'exit']) {
    assertScope(`${builtin}(true)`, builtin, 'support.function.builtin.candy');
  }
  const source = fs.readFileSync(path.resolve(root, '../../src/core/candy_keyword.list'), 'utf8');
  for (const match of source.matchAll(/^CANDY_KW\(\s*(\w+),/gm)) {
    const keyword = match[1];
    assertScope(keyword, keyword, ['true', 'false', 'null'].includes(keyword)
      ? 'constant.language.candy' : 'keyword.control.candy');
  }
  assertScope('gift = true_value', 'true_value', 'variable.other.candy');
});

test('highlights numeric bases, floats and scientific notation', () => {
  for (const number of ['0', '42', '077', '0xff', '0XAB', '0b101', '0B10', '3.14', '1.', '1e-3', '2.5E+6']) {
    assertScope(`x = ${number}`, number, 'constant.numeric.candy');
  }
});

test('highlights all lexer operators', () => {
  const source = fs.readFileSync(path.resolve(root, '../../src/core/candy_operator.list'), 'utf8');
  for (const match of source.matchAll(/^CANDY_OP\(\s*\w+,\s*(.+)\)/gm)) {
    const operator = [...match[1].matchAll(/'(.)'/g)].map(part => part[1]).join('');
    const scope = /[()[\]{}]/.test(operator) ? 'punctuation.section.brackets.candy'
      : /^[,. :]$/.test(operator) ? 'punctuation.separator.candy' : 'keyword.operator.candy';
    assertScope(`a ${operator} b`, operator, scope);
  }
});

test('comments shield keywords and quotes; strings shield hash comments', () => {
  assertScope('# if print("hi")', 'if', 'comment.line.number-sign.candy');
  assertScope('print("# end")', '# end', 'string.quoted.double.candy');
  assertScope("s = 'if # end'", 'if # end', 'string.quoted.single.candy');
  for (const escape of ['\\n', '\\x41', '\\041', '\\"']) {
    assertScope(`s = "${escape}"`, escape, 'constant.character.escape.candy');
  }
  assertScope('s = "say \\"hello\\"" # end', '# end', 'comment.line.number-sign.candy');
});

test('triple-quoted strings preserve state and close on their own delimiter', () => {
  for (const [quote, scope] of [
    ['"""', 'string.quoted.triple.double.candy'],
    ["'''", 'string.quoted.triple.single.candy']
  ]) {
    let state = tokens(`s = ${quote}hello`).ruleStack;
    state = assertScope('# if "still text"', '# if', scope, state);
    assertScope(`${quote} # done`, '# done', 'comment.line.number-sign.candy', state);
  }
});

test('single-line strings recover at newline and accept escaped newline', () => {
  const unclosed = tokens('s = "unfinished').ruleStack;
  assertScope('if (true)', 'if', 'keyword.control.candy', unclosed);
  const continued = tokens('s = "continued\\').ruleStack;
  assertScope('text" # done', 'text', 'string.quoted.double.candy', continued);
  const escapedSlash = tokens('s = "unfinished\\\\').ruleStack;
  assertScope('if (true)', 'if', 'keyword.control.candy', escapedSlash);
});

test('tokenizes a representative script and recognizes function names', () => {
  let state = INITIAL;
  for (const line of SAMPLE.split(/\r?\n/)) {
    if (/^def /.test(line)) {
      assertScope(line, line.match(/^def (\w+)/)[1], 'entity.name.function.candy', state);
    }
    const result = tokens(line, state);
    assert.ok(result.tokens.length > 0);
    state = result.ruleStack;
  }
});

test('manifest wires existing resources, command, keybinding and trust protection', () => {
  const manifest = json('package.json');
  assert.deepEqual(manifest.contributes.languages[0].extensions, ['.candy']);
  for (const file of [
    manifest.main,
    manifest.contributes.languages[0].configuration,
    manifest.contributes.grammars[0].path,
    manifest.contributes.snippets[0].path
  ]) {
    assert.ok(fs.existsSync(path.join(root, file)));
  }
  const command = manifest.contributes.commands[0];
  assert.equal(command.command, 'candy.runFile');
  assert.equal(command.icon, '$(play)');
  assert.match(command.enablement, /isWorkspaceTrusted/);
  assert.equal(manifest.contributes.keybindings[0].command, command.command);
  assert.ok(manifest.capabilities.untrustedWorkspaces.restrictedConfigurations.includes('candy.executablePath'));
});

test('language configuration covers comments, block indentation and folding', () => {
  const config = json('language-configuration.json');
  assert.equal(config.comments.lineComment, '#');
  const increase = new RegExp(config.indentationRules.increaseIndentPattern);
  const decrease = new RegExp(config.indentationRules.decreaseIndentPattern);
  for (const line of ['def f(x)', 'if (true)', 'elif (false)', 'else', 'while (true)', 'for (x : xs)', 'a = [', 't = {']) {
    assert.ok(increase.test(line), line);
  }
  for (const line of ['end', 'elif (true)', 'else', ']', '}', ')']) {
    assert.ok(decrease.test(line), line);
  }
  for (const line of ['# if (true)', 'print("end")', 'gift = 1']) {
    assert.ok(!increase.test(line), line);
    assert.ok(!decrease.test(line), line);
  }
  const regionStart = new RegExp(config.folding.markers.start);
  const regionEnd = new RegExp(config.folding.markers.end);
  for (const line of ['#region Solver', '  # region Solver']) {
    assert.ok(regionStart.test(line), line);
  }
  for (const line of ['#endregion', '  # endregion']) {
    assert.ok(regionEnd.test(line), line);
  }
  for (const line of ['def solve(g)', 'if (true)', 'while (true)', 'for (x : xs)', 'end', '# regional notes']) {
    assert.ok(!regionStart.test(line), line);
    assert.ok(!regionEnd.test(line), line);
  }
  for (const line of SAMPLE.split(/\r?\n/)) {
    assert.ok(!regionStart.test(line), `Ordinary code must not become a minimap region header: ${line}`);
  }
});

test('snippets use Candy blocks and range-based for syntax', () => {
  const snippets = Object.values(json('snippets/candy.json'));
  assert.deepEqual(snippets.map(s => s.prefix).sort(), ['def', 'elif', 'for', 'if', 'ifelse', 'print', 'while']);
  for (const snippet of snippets.filter(s => ['def', 'if', 'ifelse', 'for', 'while'].includes(s.prefix))) {
    assert.equal(snippet.body.at(-1), 'end');
    assert.ok(snippet.body.includes('  $0'));
  }
  assert.match(snippets.find(s => s.prefix === 'for').body[0], / : /);
});
