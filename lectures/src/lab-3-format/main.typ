#import "/template/lecture.typ": *

// Лабораторная работа, а не лекция: у неё свой номер, она относится к
// лекции 6, и контрольных вопросов с задачником у неё нет — задание само по
// себе является задачей.
#show: lecture.with(
  lab-id: "lab-3-format",
  questions: false,
  tasks: false,
)

#include "main.body.typ"
