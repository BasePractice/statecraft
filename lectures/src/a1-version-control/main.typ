#import "/template/lecture.typ": *

// Приложение курса, а не лекция: у него нет номера в расписании, нет
// контрольных вопросов и задач — это справка по инструменту, которым
// сдаются лабораторные работы.
#show: lecture.with(
  appendix-id: "a1-version-control",
  questions: false,
  tasks: false,
  listings-list: false,
)

#include "main.body.typ"
