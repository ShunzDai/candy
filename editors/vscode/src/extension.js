'use strict';

const vscode = require('vscode');
const { createRunFileCommand } = require('./run-file');

function activate(context) {
  context.subscriptions.push(
    vscode.commands.registerCommand('candy.runFile', createRunFileCommand(vscode))
  );
}

module.exports = { activate };
