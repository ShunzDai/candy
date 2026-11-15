'use strict';

const path = require('node:path');

function createRunFileCommand(vscode) {
  return async function runFile() {
    try {
      if (!vscode.workspace.isTrusted) {
        throw new Error('Trust this workspace before running Candy scripts.');
      }
      const document = vscode.window.activeTextEditor?.document;
      if (!document || document.languageId !== 'candy') {
        throw new Error('Open a Candy (.candy) file before running it.');
      }
      if (document.isUntitled || document.uri.scheme !== 'file') {
        throw new Error('Save the Candy script to a file on disk before running it.');
      }
      if (document.isDirty && !(await document.save())) {
        throw new Error('The Candy script could not be saved. Execution was cancelled.');
      }
      const folder = vscode.workspace.getWorkspaceFolder(document.uri);
      const configured = vscode.workspace
        .getConfiguration('candy', document.uri)
        .get('executablePath', 'candy');
      if (typeof configured !== 'string' || !configured.trim()) {
        throw new Error('Set candy.executablePath to a Candy interpreter executable.');
      }
      const cwd = path.dirname(document.uri.fsPath);
      // Bare names use PATH; relative paths belong to the script's workspace.
      const executable = path.isAbsolute(configured) || !/[\\/]/.test(configured)
        ? configured
        : path.resolve(folder?.uri.fsPath ?? cwd, configured);
      const task = new vscode.Task(
        { type: 'candy', file: document.uri.fsPath },
        folder ?? vscode.TaskScope.Global,
        `Run ${path.basename(document.uri.fsPath)}`,
        'Candy',
        new vscode.ProcessExecution(executable, [document.uri.fsPath], { cwd }),
        []
      );
      task.presentationOptions = {
        reveal: vscode.TaskRevealKind.Always,
        panel: vscode.TaskPanelKind.Dedicated,
        focus: true,
        showReuseMessage: false
      };
      await vscode.tasks.executeTask(task);
    } catch (error) {
      await vscode.window.showErrorMessage(
        `Candy: ${error instanceof Error ? error.message : String(error)}`
      );
    }
  };
}

module.exports = { createRunFileCommand };
