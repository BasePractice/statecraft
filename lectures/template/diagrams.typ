// lectures/template/diagrams.typ — векторные диаграммы автоматов.
//
// ЕДИНСТВЕННЫЙ файл шаблона с внешними зависимостями. Базовый шаблон
// (template/lecture.typ) его не импортирует, поэтому лекция без диаграмм
// собирается офлайн и без сети. Лекция, которой диаграммы нужны, импортирует
// этот файл явно:
//
//   #import "/template/diagrams.typ": fsm-diagram
//
// Перед первой сборкой прогрейте локальный кэш пакетов:
//   ./scripts/vendor-packages.sh

#import "@preview/fletcher:0.5.8" as fletcher: diagram, edge, node
#import "theme.typ": fonts, palette, sizes
#import "i18n.typ": L

// Диаграмма Мура: узлы-состояния и помеченные дуги.
//
// states — массив (id: "0", pos: (x, y), label: [0], initial: false)
// arcs   — массив (from: "0", to: "1", label: [(1, 0)], bend: 20deg)
#let fsm-diagram(
  states,
  arcs,
  caption: none,
  spacing: 3.2cm,
  label-size: 9pt,
) = {
  let body = diagram(
    node-stroke: 0.7pt + palette.ink,
    node-shape: circle,
    node-inset: 7pt,
    node-outset: 2pt,
    spacing: spacing,
    edge-stroke: 0.6pt + palette.ink,
    label-size: label-size,
    {
      for s in states {
        node(
          s.pos,
          text(font: fonts.text, size: 11pt, s.label),
          name: label(s.id),
          radius: 0.42cm,
          extrude: if s.at("initial", default: false) { (0, 4) } else { (0,) },
        )
      }
      for a in arcs {
        // loop-angle принимается только для петель; для обычных дуг
        // fletcher 0.5.8 падает на значении auto.
        if a.from == a.to {
          edge(
            label(a.from),
            label(a.to),
            a.at("label", default: none),
            "-|>",
            loop-angle: a.at("loop", default: 90deg),
            // без явного bend петля рисуется внутри узла и накрывает подпись
            bend: a.at("bend", default: 130deg),
            label-sep: 3pt,
          )
        } else {
          edge(
            label(a.from),
            label(a.to),
            a.at("label", default: none),
            "-|>",
            bend: a.at("bend", default: 0deg),
            label-pos: a.at("pos", default: 0.5),
            label-sep: 3pt,
          )
        }
      }
    },
  )
  figure(body, caption: caption)
}

// ---------------------------------------------------------------------------
// Структурные (автоматные) схемы: блоки, шины, обратные связи.
// Рисуются на CeTZ; функции draw доступны через `cetz.draw`.
// ---------------------------------------------------------------------------

#import "@preview/cetz:0.3.4"

// Обёртка: холст CeTZ внутри figure с подписью.
#let scheme(body, caption: none, length: 1cm, label: none) = {
  let f = figure(
    cetz.canvas(length: length, body),
    caption: caption,
  )
  if label == none { f } else { [#f #label] }
}

// Прямоугольный функциональный элемент с подписью внутри.
#let block(sw, ne, name: none, label: none, ..style) = {
  import cetz.draw: content, rect
  rect(sw, ne, name: name, ..style)
  if label != none { content(name + ".center", label) }
}
