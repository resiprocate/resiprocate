BUILD = ../build

include $(BUILD)/Makefile.pre

QT_LIBNAME = qt
QT_LIBDIRS = /usr/lib/qt-3.0.5/lib
QT_INCLUDEDIRS = /usr/lib/qt-3.0.5/include
NCURSES_LIBNAME = ncurses
PACKAGES += RESIPROCATE ARES PTHREAD NCURSES QT OPENSSL

TESTPROGRAMS =	limpetx.cxx

SRC = \
		Limpet.cxx \
		moc_Limpet.cxx \
		SetupDlg.cxx \
		moc_SetupDlg.cxx \
		LimpetImpl.cxx \
		moc_LimpetImpl.cxx \
		SetupDlgImpl.cxx \
		moc_SetupDlgImpl.cxx

lclean:
	rm SetupDlg.?xx Limpet.?xx moc_* 

SetupDlg.hxx: setup.ui
		uic -o SetupDlg.hxx setup.ui

SetupDlg.cxx: setup.ui SetupDlg.hxx
		uic -i SetupDlg.hxx -o SetupDlg.cxx setup.ui

Limpet.hxx: setup.ui
		uic -o Limpet.hxx limpet.ui

Limpet.cxx: setup.ui Limpet.hxx
		uic -i Limpet.hxx -o Limpet.cxx limpet.ui

moc_Limpet.cxx: Limpet.hxx
		moc -o moc_Limpet.cxx Limpet.hxx

moc_LimpetImpl.cxx: LimpetImpl.hxx
		moc -o moc_LimpetImpl.cxx LimpetImpl.hxx

moc_SetupDlg.cxx: SetupDlg.hxx
		moc -o moc_SetupDlg.cxx SetupDlg.hxx

moc_SetupDlgImpl.cxx: SetupDlgImpl.hxx
		moc -o moc_SetupDlgImpl.cxx SetupDlgImpl.hxx

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif

include $(BUILD)/Makefile.post

