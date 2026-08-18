# lectures/Makefile — единственный вход в сборку лекций.
#
#   make            собрать все лекции и разложить копии под читаемыми
#                   именами (то же, что make pretty)
#   make build      только id-шные PDF, без читаемых копий
#   make check      проверить окружение
#   make fix        проверить и доустановить недостающее (шрифты, пакеты)
#   make fonts      скачать шрифты в fonts/
#   make sync       подтянуть листинги лекций из practices/
#   make sync-check проверить, что листинги не разошлись с practices/
#   make highlight  проверить подсветку листингов в собранных PDF
#   make version    напечатать версию курса из course.typ (она же версия релиза)
#   make pack       комплект для раздачи: PDF под читаемыми именами в zip
#   make watch L=03-synthesis   пересборка одной лекции при изменении
#   make one L=03-synthesis     собрать одну лекцию
#   make book       только сводный том: все лекции и приложения одним PDF
#   make pretty     собрать и разложить копии под читаемыми именами
#   make draft      собрать с водяным знаком «ЧЕРНОВИК»
#   make clean      очистить out/
#   make distclean  очистить out/, fonts/ и .typst-packages/
#
# OUT=<каталог> переносит результат: этим пользуется сборка из CMake, чтобы
# PDF попадали в build/lectures. Порядок и параметры сборки при этом те же —
# CMake ничего не дублирует, а вызывает эти же цели.
#
#   make OUT=/tmp/pdf pretty

SHELL := /usr/bin/env bash
S := ./scripts

OUT ?= out
export OUT_DIR := $(abspath $(OUT))

.PHONY: all check fix fonts build one book watch pretty draft sync sync-check highlight version pack clean distclean help

all: pretty

build:
	@$(S)/build.sh

one:
	@test -n "$(L)" || { echo "укажите лекцию: make one L=03-synthesis"; exit 1; }
	@$(S)/build.sh "$(L)"

book:
	@$(S)/build.sh --book --pretty

watch:
	@test -n "$(L)" || { echo "укажите лекцию: make watch L=03-synthesis"; exit 1; }
	@$(S)/build.sh --watch "$(L)"

pretty:
	@$(S)/build.sh --pretty

draft:
	@$(S)/build.sh --draft

check:
	@$(S)/check.sh

fix:
	@$(S)/check.sh --fix

fonts:
	@$(S)/fetch-fonts.sh

sync:
	@$(S)/sync-code.sh

sync-check:
	@$(S)/sync-code.sh --check

highlight:
	@$(S)/check-highlight.py

version:
	@$(S)/course.sh --version

pack:
	@$(S)/pack.sh

clean:
	@$(S)/clean.sh

distclean:
	@$(S)/clean.sh --all

help:
	@sed -n '3,26p' Makefile | sed 's/^# \{0,1\}//'
