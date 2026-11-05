const vscode = require('vscode');
const crypto = require('crypto');

const escapeHtml = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

/**
 * 「クラス名」と「継承元」を同時に入力するダイアログ(Visual Studio の「クラスの追加」風)
 * @param location 作成先の表示用パス
 * @param validate ({ name, baseName }) => エラー文字列 | undefined
 * @returns { name, baseName } | undefined (キャンセル時)
 */
function askClass(location, validate) {
    const panel = vscode.window.createWebviewPanel('novawingNewClass', '新しいクラス', vscode.ViewColumn.Active, { enableScripts: true });
    panel.webview.html = buildHtml(location);

    return new Promise((resolve) => {
        let answered = false;
        const finish = (result) => {
            answered = true;
            resolve(result);
            panel.dispose();
        };

        panel.webview.onDidReceiveMessage((message) => {
            if (message.type === 'cancel') return finish(undefined);
            const input = { name: message.name.trim(), baseName: message.baseName.trim() };
            const error = validate(input);
            if (error) panel.webview.postMessage({ type: 'error', text: error });
            else finish(input);
        });
        panel.onDidDispose(() => {
            if (!answered) resolve(undefined);
        });
    });
}

function buildHtml(location) {
    const nonce = crypto.randomBytes(16).toString('hex');
    return `<!DOCTYPE html>
<html lang="ja">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'nonce-${nonce}'; script-src 'nonce-${nonce}';">
<style nonce="${nonce}">
    body { font-family: var(--vscode-font-family); color: var(--vscode-foreground); padding: 24px; max-width: 480px; }
    h2 { font-weight: normal; margin-top: 0; }
    label { display: block; margin: 16px 0 4px; }
    input { width: 100%; box-sizing: border-box; padding: 6px 8px; color: var(--vscode-input-foreground); background: var(--vscode-input-background); border: 1px solid var(--vscode-input-border, transparent); }
    input:focus { outline: 1px solid var(--vscode-focusBorder); }
    .hint { color: var(--vscode-descriptionForeground); font-size: 0.9em; margin-top: 4px; }
    #error { color: var(--vscode-errorForeground); min-height: 1.4em; margin-top: 16px; }
    .buttons { margin-top: 8px; display: flex; gap: 8px; }
    button { padding: 6px 16px; border: none; cursor: pointer; color: var(--vscode-button-foreground); background: var(--vscode-button-background); }
    button:hover { background: var(--vscode-button-hoverBackground); }
    button.secondary { color: var(--vscode-button-secondaryForeground); background: var(--vscode-button-secondaryBackground); }
</style>
</head>
<body>
    <h2>新しいクラス</h2>
    <div class="hint">作成先: ${escapeHtml(location)}</div>
    <label for="name">クラス名</label>
    <input id="name" autofocus>
    <label for="base">継承元のクラス名(省略可)</label>
    <input id="base">
    <div class="hint">空欄なら継承しません。ヘッダーはプロジェクト内から自動で探します。</div>
    <div id="error"></div>
    <div class="buttons">
        <button id="ok">作成</button>
        <button id="cancel" class="secondary">キャンセル</button>
    </div>
<script nonce="${nonce}">
    const vscode = acquireVsCodeApi();
    const nameInput = document.getElementById('name');
    const baseInput = document.getElementById('base');
    const submit = () => vscode.postMessage({ type: 'submit', name: nameInput.value, baseName: baseInput.value });
    document.getElementById('ok').addEventListener('click', submit);
    document.getElementById('cancel').addEventListener('click', () => vscode.postMessage({ type: 'cancel' }));
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') submit();
        if (e.key === 'Escape') vscode.postMessage({ type: 'cancel' });
    });
    window.addEventListener('message', (e) => {
        if (e.data.type === 'error') document.getElementById('error').textContent = e.data.text;
    });
    nameInput.focus();
</script>
</body>
</html>`;
}

module.exports = { askClass };
