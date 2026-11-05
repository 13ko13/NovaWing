const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const templates = require('./templates');
const { askClass } = require('./classDialog');

const IDENTIFIER = /^[A-Za-z_]\w*$/;

// MSVC が日本語コメントを読めるよう、BOM つき UTF-8・CRLF で書く(既存ファイルに合わせる)
function writeSource(filePath, lines) {
    fs.writeFileSync(filePath, '﻿' + lines.join('\r\n') + '\r\n', 'utf8');
}

const isInside = (child, parent) => child.toLowerCase().startsWith(parent.toLowerCase() + path.sep);

/** ファイルの作成・移動・改名・削除と、vcxproj への反映をひとまとめにしたもの */
class Operations {
    constructor(rootDir, project, afterChange) {
        this.rootDir = rootDir;
        this.project = project;
        this.afterChange = afterChange;
    }

    async newClass(destDir) {
        const location = path.relative(this.rootDir, destDir) || path.basename(this.rootDir);
        const input = await askClass(location, ({ name, baseName }) => {
            if (!IDENTIFIER.test(name)) return 'クラス名は英数字とアンダースコアのみ(数字始まり不可)';
            if (baseName && !IDENTIFIER.test(baseName)) return '継承元は英数字とアンダースコアのみ';
            if (fs.existsSync(path.join(destDir, `${name}.h`)) || fs.existsSync(path.join(destDir, `${name}.cpp`))) return '同名のファイルが既にあります';
            return undefined;
        });
        if (!input) return;
        const { name, baseName } = input;
        const base = baseName ? await this.#resolveBase(baseName) : undefined;

        const header = path.join(destDir, `${name}.h`);
        const source = path.join(destDir, `${name}.cpp`);
        writeSource(header, templates.classHeader(name, base));
        writeSource(source, templates.classSource(name));
        this.project.addFile(header);
        this.project.addFile(source);
        this.afterChange();
        await vscode.window.showTextDocument(vscode.Uri.file(header));
    }

    async newShader(destDir) {
        const kind = await vscode.window.showQuickPick(templates.SHADER_KINDS, { placeHolder: '作成する HLSL の種類' });
        if (!kind) return;
        const input = await this.#askName('ファイル名(PS / VS は自動で末尾に付きます)', (value) =>
            IDENTIFIER.test(value) ? undefined : '英数字とアンダースコアのみ');
        if (!input) return;
        const baseName = input.endsWith(kind.suffix) ? input : input + kind.suffix;
        const filePath = path.join(destDir, baseName + kind.ext);
        if (fs.existsSync(filePath)) {
            vscode.window.showErrorMessage(`${path.basename(filePath)} は既にあります`);
            return;
        }
        writeSource(filePath, kind.lines());
        this.project.addFile(filePath);
        this.afterChange();
        await vscode.window.showTextDocument(vscode.Uri.file(filePath));
    }

    async newFolder(destDir) {
        const name = await this.#askName('フォルダー名', (value) =>
            fs.existsSync(path.join(destDir, value)) ? '同名のフォルダーが既にあります' : undefined);
        if (!name) return;
        fs.mkdirSync(path.join(destDir, name));
        this.afterChange();
    }

    async rename(target) {
        const oldName = path.basename(target);
        const newName = await this.#askName('新しい名前', (value) =>
            value !== oldName && fs.existsSync(path.join(path.dirname(target), value)) ? '同名のものが既にあります' : undefined, oldName);
        if (!newName || newName === oldName) return;
        const dest = path.join(path.dirname(target), newName);
        fs.renameSync(target, dest);
        this.project.renamePath(target, dest);
        this.afterChange();
    }

    async remove(targets) {
        const names = targets.map((t) => path.basename(t)).join(', ');
        const answer = await vscode.window.showWarningMessage(
            `${names} を削除しますか?(ごみ箱へ移動します)`, { modal: true }, '削除');
        if (answer !== '削除') return;
        for (const target of targets) {
            await vscode.workspace.fs.delete(vscode.Uri.file(target), { recursive: true, useTrash: true });
            this.project.removePath(target);
        }
        this.afterChange();
    }

    /** targets を destDir の直下へ移動する(ドラッグ&ドロップ・貼り付け・最上位へ移動で共通) */
    move(targets, destDir) {
        // 親フォルダごと選択されているものは、親の移動に含まれるので除く
        const roots = targets.filter((t) => !targets.some((other) => other !== t && isInside(t, other)));
        let movedSource = false;
        for (const target of roots) {
            const dest = path.join(destDir, path.basename(target));
            if (path.dirname(target) === destDir) continue;
            if (target === destDir || isInside(destDir, target)) {
                vscode.window.showErrorMessage(`${path.basename(target)} は自分自身の中へ移動できません`);
                continue;
            }
            if (fs.existsSync(dest)) {
                vscode.window.showErrorMessage(`移動先に同名の ${path.basename(target)} があります`);
                continue;
            }
            fs.renameSync(target, dest);
            this.project.renamePath(target, dest);
            movedSource = true;
        }
        this.afterChange();
        if (movedSource) {
            vscode.window.showInformationMessage('移動しました。他のファイルの #include のパスは自動では書き換わりません。');
        }
    }

    include(targets) {
        let count = 0;
        for (const target of targets) {
            count += fs.statSync(target).isDirectory() ? this.project.addDirectory(target) : Number(this.project.addFile(target));
        }
        this.afterChange();
        vscode.window.setStatusBarMessage(`${count} 個のファイルをプロジェクトに含めました`, 4000);
    }

    exclude(targets) {
        for (const target of targets) this.project.removePath(target);
        this.afterChange();
    }

    // 継承元クラスのヘッダーをプロジェクト内から探して、#include 用のパスを決める
    async #resolveBase(baseName) {
        const found = this.#findFiles(this.rootDir, `${baseName}.h`.toLowerCase());
        if (found.length === 0) {
            vscode.window.showWarningMessage(`${baseName}.h が見つからないため、#include は書きませんでした`);
            return { name: baseName };
        }
        const picked = found.length === 1 ? found[0]
            : await vscode.window.showQuickPick(found.map((f) => path.relative(this.rootDir, f)), { placeHolder: `${baseName}.h が複数あります` })
                .then((rel) => rel && path.join(this.rootDir, rel));
        if (!picked) return { name: baseName };
        return { name: baseName, includePath: path.relative(this.rootDir, picked).split(path.sep).join('/') };
    }

    #findFiles(dir, lowerFileName) {
        const excluded = new Set(vscode.workspace.getConfiguration('novawingSolution').get('exclude'));
        const found = [];
        for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
            if (excluded.has(entry.name)) continue;
            const full = path.join(dir, entry.name);
            if (entry.isDirectory()) found.push(...this.#findFiles(full, lowerFileName));
            else if (entry.name.toLowerCase() === lowerFileName) found.push(full);
        }
        return found;
    }

    #askName(prompt, validate, value) {
        return vscode.window.showInputBox({
            prompt,
            value,
            validateInput: (input) => (input.trim() === '' ? '名前を入力してください' : /[\\/:*?"<>|]/.test(input) ? '使えない文字があります' : validate(input)),
        });
    }
}

module.exports = { Operations };
