#import "/template/lecture.typ": *

// Приложение курса, а не лекция: справка по стандарту языка, на котором
// написан практикум. Номера в расписании нет, контрольных вопросов и задач
// тоже — это справочник, к которому возвращаются по мере надобности.
#show: lecture.with(
  appendix-id: "a3-c-standard",
  questions: false,
  tasks: false,
)

#include "main.body.typ"
