# 11-takt — модели на языке Takt

Практика к лекции 11. Модели автоматов описаны на [Takt](https://github.com/BasePractice/BuT)
(DSL для конечных автоматов, компилятор на Rust), компилируются в C и
запускаются из драйвера на C.

## Что здесь

| Файл | Что показывает |
|---|---|
| `model/traffic.takt` | светофор: `enter`/`always`, выдержки в тактах, выходной порт |
| `model/watchdog.takt` | сторожевой таймер: входной и выходной порты, возврат из аварии |
| `model/button_led.takt` | `extern fn` и функция языка, направленные порты |
| `model/pump.takt` | параллельная композиция `Pump \| Alarm` |
| `model/cooling_bad.takt` | контрпример: состояние не удерживает управление |
| `model/cooling_good.takt` | исправление: непересекающиеся и покрывающие условия выхода |
| `model/recovery.takt` | LTL-свойство `G (Fault -> F Idle)` для `taktc verify` |
| `main.c` | драйвер: реализует порты модели через callback'и порождённого кода |

## Сборка

Компилятор `taktc` внешний: подпроект молча пропускается, если его нет.

```bash
cmake -S . -B build -DSTATECRAFT_TAKTC=/путь/к/BuT/target/release
cmake --build build --target 11-takt-watchdog
ctest --test-dir build -R 11-takt
```

Цель `11-takt-models` компилирует все модели каталога — она же служит
проверкой, что примеры лекции не разошлись с языком.

## Почему этот код не C90

Всё остальное в `practices/` собирается как ISO C90 без предупреждений.
Здесь исключение: `taktc` порождает C99 (`uint8_t`, `bool`, комментарии `//`),
а драйвер обязан включить порождённый заголовок. Обе цели помечены
`statecraft_generated_c99()` — это единственная документированная поблажка,
и причина у неё внешняя.

## Ручные проверки

```bash
taktc compile -t c        model/watchdog.takt -o out/   # C
taktc compile -t st       model/traffic.takt  -o out/   # Structured Text (ПЛК)
taktc compile -t plantuml model/traffic.takt  -o out/   # диаграмма состояний
taktc verify              model/recovery.takt           # проверка LTL-свойства
takt-sim -n 40            model/traffic.takt            # потактовая трасса
```
