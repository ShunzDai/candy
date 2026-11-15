'use strict';

const assert = require('node:assert/strict');
const { test } = require('node:test');
const path = require('node:path');
const { createRunFileCommand } = require('../src/run-file');

function fixture() {
  const errors = [];
  const tasks = [];
  const folder = { uri: { fsPath: path.resolve('/workspace/second') } };
  const document = {
    languageId: 'candy',
    uri: { scheme: 'file', fsPath: path.join(folder.uri.fsPath, 'examples', "a b;$(echo x)'s.candy") },
    isUntitled: false,
    isDirty: false,
    async save() { return true; }
  };
  const state = { executable: 'candy', folder };
  const vscode = {
    workspace: {
      isTrusted: true,
      getWorkspaceFolder(uri) {
        assert.equal(uri, document.uri);
        return state.folder;
      },
      getConfiguration(section, uri) {
        assert.equal(section, 'candy');
        assert.equal(uri, document.uri);
        return {
          get(key, fallback) {
            assert.equal(key, 'executablePath');
            assert.equal(fallback, 'candy');
            return state.executable;
          }
        };
      }
    },
    window: {
      activeTextEditor: { document },
      async showErrorMessage(message) { errors.push(message); }
    },
    ProcessExecution: class {
      constructor(executable, args, options) {
        Object.assign(this, { executable, args, options });
      }
    },
    Task: class {
      constructor(definition, scope, name, source, execution, problemMatchers) {
        Object.assign(this, { definition, scope, name, source, execution, problemMatchers });
      }
    },
    TaskScope: { Global: 1 },
    TaskRevealKind: { Always: 1 },
    TaskPanelKind: { Dedicated: 2 },
    tasks: { async executeTask(task) { tasks.push(task); } }
  };
  return { vscode, document, state, errors, tasks, run: createRunFileCommand(vscode) };
}

test('executes with separate arguments, script cwd and visible dedicated terminal', async () => {
  const f = fixture();
  await f.run();
  assert.deepEqual(f.errors, []);
  assert.equal(f.tasks.length, 1);
  const task = f.tasks[0];
  assert.equal(task.execution.executable, 'candy');
  assert.deepEqual(task.execution.args, [f.document.uri.fsPath]);
  assert.equal(task.execution.options.cwd, path.dirname(f.document.uri.fsPath));
  assert.equal(task.scope, f.state.folder);
  assert.equal(task.presentationOptions.reveal, f.vscode.TaskRevealKind.Always);
  assert.equal(task.presentationOptions.panel, f.vscode.TaskPanelKind.Dedicated);
});

test('resolves relative interpreter from containing workspace, not script cwd', async () => {
  const f = fixture();
  f.state.executable = 'build/shell/candy';
  await f.run();
  assert.equal(f.tasks[0].execution.executable, path.join(f.state.folder.uri.fsPath, 'build/shell/candy'));
});

test('keeps an absolute interpreter path with spaces intact', async () => {
  const f = fixture();
  f.state.executable = path.resolve('/tools/candy interpreter');
  await f.run();
  assert.equal(f.tasks[0].execution.executable, f.state.executable);
});

test('runs standalone files with global scope and script-relative interpreter', async () => {
  const f = fixture();
  f.state.folder = undefined;
  f.state.executable = './candy';
  await f.run();
  assert.equal(f.tasks[0].scope, f.vscode.TaskScope.Global);
  assert.equal(f.tasks[0].execution.executable, path.join(path.dirname(f.document.uri.fsPath), 'candy'));
});

test('saves dirty files before starting task', async () => {
  const f = fixture();
  f.document.isDirty = true;
  let saved = false;
  f.document.save = async () => { saved = true; return true; };
  f.vscode.tasks.executeTask = async (task) => {
    assert.ok(saved);
    f.tasks.push(task);
  };
  await f.run();
  assert.equal(f.tasks.length, 1);
});

for (const [name, mutate, expected] of [
  ['untrusted workspace', f => { f.vscode.workspace.isTrusted = false; }, /Trust this workspace/],
  ['missing editor', f => { f.vscode.window.activeTextEditor = undefined; }, /Open a Candy/],
  ['wrong language', f => { f.document.languageId = 'plaintext'; }, /Open a Candy/],
  ['untitled file', f => { f.document.isUntitled = true; }, /Save the Candy script/],
  ['virtual file', f => { f.document.uri.scheme = 'vscode-vfs'; }, /Save the Candy script/],
  ['empty executable', f => { f.state.executable = '  '; }, /candy.executablePath/],
  ['invalid executable type', f => { f.state.executable = 42; }, /candy.executablePath/],
  ['failed save', f => {
    f.document.isDirty = true;
    f.document.save = async () => false;
  }, /could not be saved/],
  ['save exception', f => {
    f.document.isDirty = true;
    f.document.save = async () => { throw new Error('disk full'); };
  }, /disk full/],
  ['task startup failure', f => {
    f.vscode.tasks.executeTask = async () => { throw new Error('interpreter missing'); };
  }, /interpreter missing/]
]) {
  test(`reports ${name} without executing`, async () => {
    const f = fixture();
    mutate(f);
    await f.run();
    assert.equal(f.tasks.length, 0);
    assert.equal(f.errors.length, 1);
    assert.match(f.errors[0], expected);
  });
}
