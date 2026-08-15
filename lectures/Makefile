# lectures/Makefile — тонкая обёртка над scripts/.
#
#   make            собрать все лекции в out/
#   make check      проверить окружение
#   make fix        проверить и доустановить недостающее (шрифты, пакеты)
#   make fonts      скачать шрифты в fonts/
#   make watch L=03-synthesis   пересборка одной лекции при изменении
#   make one L=03-synthesis     собрать одну лекцию
#   make pretty     собрать и разложить копии под читаемыми именами
#   make draft      собрать с водяным знаком «ЧЕРНОВИК»
#   make clean      очистить out/
#   make distclean  очистить out/, fonts/ и .typst-packages/

SHELL := /usr/bin/env bash
S := ./scripts

.PHONY: all check fix fonts build one watch pretty draft clean distclean help

all: build

build:
	@$(S)/build.sh

one:
	@test -n "$(L)" || { echo "укажите лекцию: make one L=03-synthesis"; exit 1; }
	@$(S)/build.sh "$(L)"

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

clean:
	@$(S)/clean.sh

distclean:
	@$(S)/clean.sh --all

help:
	@sed -n '3,15p' Makefile | sed 's/^# \{0,1\}//'
