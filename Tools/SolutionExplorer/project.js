const fs = require('fs');
const path = require('path');

const KIND_BY_EXT = {
    '.cpp': 'ClCompile',
    '.c': 'ClCompile',
    '.h': 'ClInclude',
    '.hpp': 'ClInclude',
    '.hlsl': 'FxCompile',
    '.hlsli': 'None',
};
const CONFIGS = ['Debug|x64', 'Debug|Win32', 'Release|Win32', 'Release|x64'];
const BACKSLASH = '\\';

const escapeXml = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

// Include 属性の値 value が rel そのもの、または rel フォルダの配下かどうか
const matches = (value, rel) => {
    const v = value.toLowerCase();
    const r = rel.toLowerCase();
    return v === r || v.startsWith(r + BACKSLASH);
};

/** .vcxproj と .vcxproj.filters のテキスト編集(BOM・改行コードは元のまま保つ) */
class Project {
    constructor(vcxprojPath) {
        this.file = vcxprojPath;
        this.filtersFile = vcxprojPath + '.filters';
        this.dir = path.dirname(vcxprojPath);
    }

    rel(abs) {
        return path.relative(this.dir, abs).split('/').join(BACKSLASH);
    }

    kindOf(abs) {
        return KIND_BY_EXT[path.extname(abs).toLowerCase()];
    }

    // vcxproj に登録済みの Include 値(小文字)の集合
    registeredSet() {
        const text = fs.readFileSync(this.file, 'utf8');
        return new Set([...text.matchAll(/Include="([^"]*)"/g)].map((m) => m[1].toLowerCase()));
    }

    isRegistered(abs) {
        return this.registeredSet().has(this.rel(abs).toLowerCase());
    }

    /** 登録できる拡張子のファイルを追加する。登録したら true */
    addFile(abs) {
        const kind = this.kindOf(abs);
        if (!kind || this.isRegistered(abs)) return false;
        const rel = escapeXml(this.rel(abs));
        this.#edit(this.file, (text, nl) => insertItem(text, kind, this.#projectXml(kind, rel, abs), nl));
        this.#edit(this.filtersFile, (text, nl) => insertItem(text, kind, [`    <${kind} Include="${rel}" />`], nl));
        return true;
    }

    /** フォルダ配下の登録できるファイルを全部追加する。追加した数を返す */
    addDirectory(absDir) {
        let count = 0;
        for (const entry of fs.readdirSync(absDir, { withFileTypes: true })) {
            const abs = path.join(absDir, entry.name);
            if (entry.isDirectory()) count += this.addDirectory(abs);
            else if (this.addFile(abs)) count++;
        }
        return count;
    }

    /** abs(ファイルまたはフォルダ)の登録を消す */
    removePath(abs) {
        const rel = this.rel(abs);
        const remove = (text) => text.replace(
            /[ \t]*<(\w+) Include="([^"]*)"(?:\s*\/>|>[\s\S]*?<\/\1>)[ \t]*\r?\n?/g,
            (whole, _tag, include) => (matches(include, rel) ? '' : whole));
        this.#edit(this.file, remove);
        this.#edit(this.filtersFile, remove);
    }

    /** oldAbs から newAbs への移動・改名を Include に反映する */
    renamePath(oldAbs, newAbs) {
        const oldRel = this.rel(oldAbs);
        const newRel = escapeXml(this.rel(newAbs));
        const rename = (text) => text.replace(/Include="([^"]*)"/g, (whole, include) =>
            matches(include, oldRel) ? `Include="${newRel}${include.slice(oldRel.length)}"` : whole);
        this.#edit(this.file, rename);
        this.#edit(this.filtersFile, rename);
    }

    #projectXml(kind, rel, abs) {
        if (kind !== 'FxCompile') return [`    <${kind} Include="${rel}" />`];
        const isVertex = /VS\.hlsl$/i.test(abs);
        const ext = isVertex ? 'vso' : 'pso';
        const lines = [`    <FxCompile Include="${rel}">`];
        for (const cfg of CONFIGS) {
            const cond = `Condition="'$(Configuration)|$(Platform)'=='${cfg}'"`;
            lines.push(`      <ShaderType ${cond}>${isVertex ? 'Vertex' : 'Pixel'}</ShaderType>`);
            lines.push(`      <ShaderModel ${cond}>5.0</ShaderModel>`);
            lines.push(`      <ObjectFileOutput ${cond}>$(ProjectDir)%(Filename).${ext}</ObjectFileOutput>`);
        }
        lines.push('    </FxCompile>');
        return lines;
    }

    #edit(file, transform) {
        const text = fs.readFileSync(file, 'utf8');
        const nl = text.includes('\r\n') ? '\r\n' : '\n';
        const result = transform(text, nl);
        if (result !== text) fs.writeFileSync(file, result, 'utf8');
    }
}

// 同じ種類の最後の項目があるグループの末尾へ。なければ新しいグループを作る
function insertItem(text, kind, lines, nl) {
    let lastIndex = -1;
    for (const m of text.matchAll(new RegExp(`<${kind} Include=`, 'g'))) lastIndex = m.index;

    if (lastIndex >= 0) {
        const groupEnd = text.indexOf('</ItemGroup>', lastIndex);
        const lineStart = text.lastIndexOf('\n', groupEnd) + 1;
        return text.slice(0, lineStart) + lines.join(nl) + nl + text.slice(lineStart);
    }

    const importIndex = text.indexOf('<Import Project="$(VCTargetsPath)' + BACKSLASH + 'Microsoft.Cpp.targets"');
    const anchor = importIndex >= 0 ? importIndex : text.lastIndexOf('</Project>');
    const lineStart = text.lastIndexOf('\n', anchor) + 1;
    const group = ['  <ItemGroup>', ...lines, '  </ItemGroup>'].join(nl) + nl;
    return text.slice(0, lineStart) + group + text.slice(lineStart);
}

module.exports = { Project };
