#import "/template/lecture.typ": *

// Приложение курса, а не лекция: справка по требованиям к коду, которым
// сдаются лабораторные и курсовая работа. Номера в расписании нет,
// контрольных вопросов и задач тоже.
#show: lecture.with(
  appendix-id: "a2-code-style",
  questions: false,
  tasks: false,
)

#include "main.body.typ"
