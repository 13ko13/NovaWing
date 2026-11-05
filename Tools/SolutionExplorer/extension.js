const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const { Project } = require('./project');
const { Operations } = require('./operations');
const { SolutionTree } = require('./tree');
const { Builder } = require('./build');

function activate(context) {
    const folder = vscode.workspace.workspaceFolders?.[0];
    if (!folder) return;
    const rootDir = folder.uri.fsPath;
    const vcxproj = fs.readdirSync(rootDir).find((name) => name.endsWith('.vcxproj'));
    if (!vcxproj) return;

    const project = new Project(path.join(rootDir, vcxproj));
    const builder = new Builder(context, project.file, folder);

    let tree;
    const ops = new Operations(rootDir, project, () => tree.refresh());
    tree = new SolutionTree(rootDir, project, (paths, destDir) => ops.move(paths, destDir));
    const view = vscode.window.createTreeView('novawingSolution', {
        treeDataProvider: tree,
        dragAndDropController: tree,
        canSelectMany: true,
        showCollapseAll: true,
    });
    context.subscriptions.push(view);

    // フォルダ構成の変化(VSCode 本体のエクスプローラーや git 操作など)も反映する
    const watcher = vscode.workspace.createFileSystemWatcher(new vscode.RelativePattern(folder, '**/*'));
    watcher.onDidCreate(() => tree.refresh());
    watcher.onDidDelete(() => tree.refresh());
    context.subscriptions.push(watcher);

    // コマンドに渡されたノード(右クリック・複数選択)か、無ければツリーの選択を対象にする
    const targetsOf = (node, nodes) => (nodes?.length ? nodes : node ? [node] : [...view.selection]);
    const pathsOf = (node, nodes) => targetsOf(node, nodes).filter((n) => !n.isRoot).map((n) => n.path);
    // 追加系は「選んだフォルダ(ファイルなら親フォルダ)」、何も選んでいなければ最上位
    const destDirOf = (node) => {
        const target = node ?? view.selection[0];
        if (!target) return rootDir;
        return target.isDir ? target.path : path.dirname(target.path);
    };

    let clipboard = [];
    const setClipboard = (paths) => {
        clipboard = paths;
        vscode.commands.executeCommand('setContext', 'novawing.hasClipboard', paths.length > 0);
    };

    const register = (command, handler) => context.subscriptions.push(vscode.commands.registerCommand(command, handler));

    register('novawing.newClass', (node) => ops.newClass(destDirOf(node)));
    register('novawing.newShader', (node) => ops.newShader(destDirOf(node)));
    register('novawing.newFolder', (node) => ops.newFolder(destDirOf(node)));
    register('novawing.include', (node, nodes) => ops.include(pathsOf(node, nodes)));
    register('novawing.exclude', (node, nodes) => ops.exclude(pathsOf(node, nodes)));
    register('novawing.moveToRoot', (node, nodes) => ops.move(pathsOf(node, nodes), rootDir));
    register('novawing.cut', (node, nodes) => setClipboard(pathsOf(node, nodes)));
    register('novawing.paste', (node) => {
        if (clipboard.length === 0) return;
        ops.move(clipboard, destDirOf(node));
        setClipboard([]);
    });
    register('novawing.rename', (node) => {
        const [target] = pathsOf(node);
        if (target) ops.rename(target);
    });
    register('novawing.delete', (node, nodes) => {
        const paths = pathsOf(node, nodes);
        if (paths.length > 0) ops.remove(paths);
    });
    register('novawing.revealInOS', (node) => vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(destDirOf(node))));
    register('novawing.openTerminal', (node) => vscode.window.createTerminal({ cwd: destDirOf(node) }).show());
    register('novawing.build', () => builder.run('Build'));
    register('novawing.rebuild', () => builder.run('Rebuild'));
    register('novawing.clean', () => builder.run('Clean'));
    register('novawing.selectConfig', () => builder.selectConfig());
    register('novawing.refresh', () => tree.refresh());
}

function deactivate() {}

module.exports = { activate, deactivate };
