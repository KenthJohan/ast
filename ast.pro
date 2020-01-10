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
SOURCES += main_test.c

INCLUDEPATH += $$PWD/csc
INCLUDEPATH += $$PWD/iup-3.28_Win64_mingw6_lib/include
HEADERS += $$PWD/iup-3.28_Win64_mingw6_lib/include/iup.h
LIBS += -L$$PWD/iup-3.28_Win64_mingw6_lib
LIBS += -liup -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32

