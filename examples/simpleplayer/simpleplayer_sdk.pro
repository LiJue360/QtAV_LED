########## template for QtAV app project BEGIN ################
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += avwidgets
} else {
  CONFIG += avwidgets
}
#rpath for osx qt4
mac {
  RPATHDIR *= @loader_path/../Frameworks @executable_path/../Frameworks
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}
########## template for QtAV app project END ################
################# Add your own code below ###################
include(src.pri)
#LIBS     += -L$$quote(C:\Qt\Qt5.14.2\5.14.2\mingw73_32\lib) -lQtAV1 -lQtAVWidgets1

QT += av avwidgets
#LIBS += -lQtAV -lQtAVWidgets
LIBS     += -L$$quote(D:\QT\My_Project\QT_LED\build-QtAV-Desktop_Qt_5_14_2_MinGW_64_bit-Release\lib_win_x86_64) -lQtAV1 -lQtAVWidgets1
LIBS += -lopengl32 -lglu32
