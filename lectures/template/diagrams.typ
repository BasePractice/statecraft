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
        // Двойной контур: у автоматов-преобразователей им отмечено начальное
        // состояние (лекции 2 и 3), у акцепторов — заключительное (лекция 4).
        let doubled = s.at("initial", default: false) or s.at("final", default: false)
        node(
          s.pos,
          // Перенос внутри узла («за-крыт») читается как две строки шума.
          text(font: fonts.diagram, size: 11pt, hyphenate: false, s.label),
          name: label(s.id),
          // Длинная подпись («G_REQ», «закрыт») не влезает в круг стандартного
          // радиуса — такому состоянию радиус задаётся полем `radius`.
          radius: s.at("radius", default: 0.42cm),
          extrude: if doubled { (0, 4) } else { (0,) },
        )
        // Свободная стрелка «вход»: начальное состояние акцептора, у которого
        // двойной контур уже занят под заключительные.
        if s.at("entry", default: false) {
          let (x, y) = s.pos
          edge((x - 0.6, y), label(s.id), "-|>")
        }
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
  // Ширину диаграммы задают координаты узлов и `spacing`, о полосе набора
  // fletcher ничего не знает: цепочка из пяти-шести состояний при spacing 3.2cm
  // уезжает за поля. Вписываем в полосу, если не помещается.
  // Все надписи рисунка — чертёжным шрифтом (ГОСТ 2.304-81): и подписи
  // состояний, и метки дуг, которые приходят готовым content. Шрифт
  // навешивается на содержимое figure, а не `set` на выходе функции: иначе
  // метка <fig:…> цепляется к styled-контенту, и ссылка на рисунок ломается.
  figure(
    text(font: fonts.diagram, layout(area => context {
      let w = measure(body).width
      if w > area.width {
        scale(x: area.width / w * 100%, y: area.width / w * 100%, reflow: true, body)
      } else {
        body
      }
    })),
    caption: caption,
  )
}

// ---------------------------------------------------------------------------
// Структурные (автоматные) схемы: блоки, шины, обратные связи.
// Рисуются на CeTZ; функции draw доступны через `cetz.draw`.
// ---------------------------------------------------------------------------

#import "@preview/cetz:0.3.4"

// Обёртка: холст CeTZ внутри figure с подписью.
#let scheme(body, caption: none, length: 1cm, label: none) = {
  let f = figure(
    text(font: fonts.diagram, cetz.canvas(length: length, body)),
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
