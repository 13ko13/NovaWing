const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');

const CONFIGS = ['Debug|x64', 'Release|x64', 'Debug|Win32', 'Release|Win32'];
const CONFIG_KEY = 'novawing.buildConfig';

function findMsBuild() {
    const vswhere = path.join(process.env['ProgramFiles(x86)'], 'Microsoft Visual Studio', 'Installer', 'vswhere.exe');
    const out = execFileSync(vswhere, [
        '-latest', '-products', '*', '-requires', 'Microsoft.Component.MSBuild',
        '-find', 'MSBuild\\**\\Bin\\MSBuild.exe',
    ], { encoding: 'utf8' });
    return out.split(/\r?\n/).find((line) => line.trim());
}

/** ビルド/リビルド/クリーンと、ステータスバーのビルド構成表示 */
class Builder {
    constructor(context, projectFile, workspaceFolder) {
        this.context = context;
        this.projectFile = projectFile;
        this.workspaceFolder = workspaceFolder;
        this.statusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 10);
        this.statusItem.command = 'novawing.selectConfig';
        this.statusItem.show();
        context.subscriptions.push(this.statusItem);
        this.#updateStatus();
    }

    get config() {
        return this.context.workspaceState.get(CONFIG_KEY, CONFIGS[0]);
    }

    run(target) {
        const msbuild = this.msbuild ??= findMsBuildOrNotify();
        if (!msbuild) return;
        const [configuration, platform] = this.config.split('|');
        const args = [this.projectFile, `/t:${target}`, `/p:Configuration=${configuration}`, `/p:Platform=${platform}`, '/m', '/v:m'];
        const strong = (value) => ({ value, quoting: vscode.ShellQuoting.Strong });
        const execution = new vscode.ShellExecution(strong(msbuild), args.map(strong));
        const label = { Build: 'ビルド', Rebuild: 'リビルド', Clean: 'クリーン' }[target];
        const task = new vscode.Task({ type: 'shell' }, this.workspaceFolder, `${label} (${this.config})`, 'NovaWing', execution, '$msCompile');
        task.presentationOptions = { reveal: vscode.TaskRevealKind.Always, clear: true };
        vscode.workspace.saveAll(false).then(() => vscode.tasks.executeTask(task));
    }

    async selectConfig() {
        const picked = await vscode.window.showQuickPick(CONFIGS, { placeHolder: 'ビルド構成を選択' });
        if (!picked) return;
        await this.context.workspaceState.update(CONFIG_KEY, picked);
        this.#updateStatus();
    }

    #updateStatus() {
        this.statusItem.text = `$(tools) ${this.config}`;
        this.statusItem.tooltip = 'ビルド構成を変更';
    }
}

function findMsBuildOrNotify() {
    try {
        const found = findMsBuild();
        if (found && fs.existsSync(found)) return found;
    } catch (e) {
        // vswhere が無い場合は下の通知へ
    }
    vscode.window.showErrorMessage('MSBuild が見つかりません。Visual Studio の「C++ によるデスクトップ開発」をインストールしてください。');
    return undefined;
}

module.exports = { Builder };
