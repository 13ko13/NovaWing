const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

const DRAG_MIME = 'application/vnd.code.tree.novawingsolution';
const SOURCE_EXTS = new Set(['.cpp', '.c', '.h', '.hpp', '.hlsl', '.hlsli']);

/** ツリーの1ノード。root はプロジェクト(最上位フォルダ)そのもの */
class Node {
    constructor(fsPath, isDir, isRoot = false) {
        this.path = fsPath;
        this.isDir = isDir;
        this.isRoot = isRoot;
    }
}

class SolutionTree {
    /**
     * @param rootDir ワークスペースの最上位フォルダ
     * @param project vcxproj 編集
     * @param onDrop (movedPaths, destDir) => void  ドラッグ&ドロップで移動するときの処理
     */
    constructor(rootDir, project, onDrop) {
        this.rootDir = rootDir;
        this.project = project;
        this.onDrop = onDrop;
        this.rootNode = new Node(rootDir, true, true);
        this.changed = new vscode.EventEmitter();
        this.onDidChangeTreeData = this.changed.event;
        this.dragMimeTypes = [DRAG_MIME];
        this.dropMimeTypes = [DRAG_MIME];
    }

    refresh() {
        this.changed.fire();
    }

    getTreeItem(node) {
        const label = node.isRoot ? path.basename(this.rootDir) : path.basename(node.path);
        const state = node.isRoot ? vscode.TreeItemCollapsibleState.Expanded
            : node.isDir ? vscode.TreeItemCollapsibleState.Collapsed
                : vscode.TreeItemCollapsibleState.None;
        const item = new vscode.TreeItem(label, state);
        item.id = node.path;
        item.resourceUri = vscode.Uri.file(node.path);

        if (node.isRoot) {
            item.contextValue = 'root';
            item.iconPath = new vscode.ThemeIcon('project');
        } else if (node.isDir) {
            item.contextValue = 'folder';
        } else {
            const unregistered = this.#isUnregistered(node.path);
            item.contextValue = unregistered ? 'fileOut' : 'file';
            item.description = unregistered ? '(未登録)' : undefined;
            item.command = { command: 'vscode.open', title: '開く', arguments: [item.resourceUri] };
        }
        return item;
    }

    getChildren(node) {
        if (!node) return [this.rootNode];
        this.registered = this.project.registeredSet();
        const excluded = new Set(vscode.workspace.getConfiguration('novawingSolution').get('exclude'));
        return fs.readdirSync(node.path, { withFileTypes: true })
            .filter((entry) => !excluded.has(entry.name))
            .map((entry) => new Node(path.join(node.path, entry.name), entry.isDirectory()))
            .sort((a, b) => (a.isDir === b.isDir ? a.path.localeCompare(b.path) : a.isDir ? -1 : 1));
    }

    handleDrag(sources, dataTransfer) {
        const paths = sources.filter((n) => !n.isRoot).map((n) => n.path);
        dataTransfer.set(DRAG_MIME, new vscode.DataTransferItem(paths));
    }

    // 空白(target なし)に落とすと最上位フォルダへ移動する
    handleDrop(target, dataTransfer) {
        const item = dataTransfer.get(DRAG_MIME);
        if (!item) return;
        const destDir = !target ? this.rootDir : target.isDir ? target.path : path.dirname(target.path);
        this.onDrop(item.value, destDir);
    }

    #isUnregistered(filePath) {
        if (!SOURCE_EXTS.has(path.extname(filePath).toLowerCase())) return false;
        return !this.registered.has(this.project.rel(filePath).toLowerCase());
    }
}

module.exports = { SolutionTree };
