TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += $$PWD/csc/csc_tcol.h
HEADERS += $$PWD/csc/csc_debug.h
HEADERS += $$PWD/csc/csc_str.h
HEADERS += $$PWD/csc/csc_tok_c.h
HEADERS += $$PWD/csc/csc_tree4.h
HEADERS += $$PWD/csc/csc_tree4_print.h
VPATH +=
SOURCES += ast2.c

INCLUDEPATH += $$PWD/csc

