// lectures/template/code.typ — оформление листингов и врезка кода из файла.

#import "theme.typ": fonts, mono-weight, palette, sizes
#import "i18n.typ": L

// Применяется show-правилами в lecture().
#let raw-show-rules(doc) = {
  // Языки, которых typst не знает из коробки. Путь абсолютный от --root.
  // Takt — язык лекции 12, Promela — входной язык SPIN (лекция 9), БНФ —
  // грамматики лексера (лекция 6). Без этого списка такие листинги
  // набирались бы одним цветом, то есть неотличимо от вывода программы.
  set raw(syntaxes: (
    "/syntaxes/takt.sublime-syntax",
    "/syntaxes/promela.sublime-syntax",
    "/syntaxes/bnf.sublime-syntax",
  ))
  // Fira Code — variable-шрифт с Default weight 300, поэтому вес задаётся явно.
  // calt: 0 гасит лигатуры (!= → ≠): в лекции по Си они мешают читать код.
  show raw: set text(
    font: fonts.mono,
    weight: mono-weight,
    features: (calt: 0),
    size: sizes.code,
  )
  show raw.where(block: true): it => block(
    width: 100%,
    breakable: true,
    fill: palette.code-bg,
    inset: (x: 9pt, y: 8pt),
    radius: 2pt,
    stroke: (left: 2pt + palette.code-rule),
  )[#it]
  show raw.line: it => context {
    if it.count > 1 {
      box(width: 2.6em, align(
        right,
        text(fill: luma(155), size: 0.85em, str(it.number)) + h(0.7em),
      ))
    }
    it.body
  }
  doc
}

// Замена \lstinputlisting и \inputminted[firstnumber=N].
//
// ВАЖНО: path должен быть абсолютным от --root, то есть начинаться со слэша:
//   #code-file("/src/10-limits/code/mt/turing_machine.h", lang: "c")
// read() внутри шаблона разрешает относительные пути от template/code.typ,
// а не от файла лекции, поэтому «code/foo.c» не найдётся.
//
// from/to — 1-based, включительно.
#let code-file(
  path,
  lang: "c",
  from: none,
  to: none,
  caption: none,
  label: none,
) = {
  let lines = read(path).split("\n")
  // Файл кончается переводом строки, поэтому split даёт последним пустой
  // элемент — без этого у каждого листинга «файл целиком» появлялась лишняя
  // пронумерованная пустая строка в конце.
  if lines.len() > 0 and lines.last() == "" { lines = lines.slice(0, -1) }
  let a = if from == none { 0 } else { from - 1 }
  let b = if to == none { lines.len() } else { to }
  let f = figure(
    align(left, raw(lines.slice(a, b).join("\n"), lang: lang, block: true)),
    caption: caption,
    kind: raw,
    supplement: [#L.listing],
  )
  if label == none { f } else { [#f #label] }
}

// Инлайновый листинг с подписью и номером.
#let listing(body, caption: none, label: none) = {
  let f = figure(
    align(left, body),
    caption: caption,
    kind: raw,
    supplement: [#L.listing],
  )
  if label == none { f } else { [#f #label] }
}
