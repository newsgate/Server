# @file   configure.ac
# @author Karen Aroutiounov
# $Id: $
 
AC_INIT([configure.ac])
 
m4_include([OpenSBE.ac])

OSBE_ENABLE_FEATURE([CXX/CXX])
OSBE_ENABLE_FEATURE([CXX/So])
OSBE_ENABLE_FEATURE([CXX/Ex])
OSBE_ENABLE_FEATURE([CXX/Inl])

OSBE_ENABLE_FEATURE([CXX/XML/XSD])
OSBE_ENABLE_FEATURE([CXX/Corba])
OSBE_ENABLE_FEATURE([Documentation/Doxygen])

python_ver=`python -V 2>&1 | sed -n -e "s/Python \(.*\)\.\(.*\)\..*/\1.\2/p"`

osbe_cxx_use_lib "Python" \
                 "Python" \
                 "include/python$python_ver" \
                 "lib" \
                 "python$python_ver" \
                 "" \
                 "External/Python.pre.rules"

osbe_cxx_use_lib "MD5" \
                 "MD5" \
                 "include/openssl" \
                 "lib" \
                 "ssl" \
                 "" \
                 "External/MD5.pre.rules"

osbe_cxx_use_lib "ZLib" \
                 "ZLib" \
                 "include" \
                 "lib" \
                 "z" \
                 "" \
                 "External/ZLib.pre.rules"

osbe_cxx_use_lib "UUID" \
                 "UUID" \
                 "include" \
                 "lib" \
                 "uuid" \
                 "" \
                 "External/UUID.pre.rules"

osbe_cxx_use_lib "DL" \
                 "DL" \
                 "include" \
                 "lib" \
                 "dl" \
                 "" \
                 "External/DL.pre.rules"

osbe_cxx_use_lib "LibXML" \
                 "LibXML" \
                 "include/libxml2" \
                 "lib" \
                 "xml2" \
                 "" \
                 "External/LibXML.pre.rules"

osbe_cxx_use_lib "Google" \
                 "Google" \
                 "include" \
                 "" \
                 "" \
                 "" \
                 "External/Google.pre.rules"

osbe_cxx_use_lib "ACE" \
                 "ACE" \
                 "include" \
                 "lib" \
                 "ACE" \
                 "" \
                 "External/ACE.pre.rules"

osbe_cxx_use_lib "Xerces" \
                 "Xerces" \
                 "include" \
                 "lib" \
                 "xerces-c" \
                 "" \
                 "External/Xerces.pre.rules"

osbe_cxx_use_lib "GeoIP" \
                 "GeoIP" \
                 "include" \
                 "lib" \
                 "GeoIP" \
                 "" \
                 "External/GeoIP.pre.rules"

osbe_cxx_use_lib "ImageMagick" \
                 "ImageMagick" \
                 "include/ImageMagick" \
                 "lib" \
                 "Magick++" \
                 "" \
                 "External/ImageMagick.pre.rules"

osbe_cxx_use_lib "TokyoCabinet" \
                 "TokyoCabinet" \
                 "include" \
                 "lib" \
                 "tokyocabinet" \
                 "" \
                 "External/TokyoCabinet.pre.rules"

osbe_cxx_use_lib "MySQL" \
                 "MySQL" \
                 "include" \
                 "lib/mysql" \
                 "mysqlclient_r z" \
                 "" \
                 "External/MySQL.pre.rules"

dnl GraphicsMagick++ GraphicsMagick lcms freetype gs Xext SM ICE X11 bz2 z m gomp pthread
dnl include/httpd - apr directory, can vary on different platforms

osbe_cxx_use_lib "APACHE" \
                 "Apache" \
                 "include include/httpd" \
                 "" \
                 "" \
                 "" \
                 "External/Apache.pre.rules"

osbe_cxx_use_lib "Elements/Basic" \
                 "ElBasic" \
                 "." \
                 "build/lib" \
                 "Elements" \
                 "" \
                 "External/ElBasic.pre.rules"

osbe_cxx_use_lib "Elements/MySQL" \
                 "ElMySQL" \
                 "." \
                 "build/lib" \
                 "ElMySQL" \
                 "" \
                 "External/ElMySQL.pre.rules"

osbe_cxx_use_lib "Elements/Net" \
                 "ElNet" \
                 "." \
                 "build/lib" \
                 "ElNet" \
                 "" \
                 "External/ElNet.pre.rules"

osbe_cxx_use_lib "Elements/Apache" \
                 "ElApache" \
                 "." \
                 "build/lib" \
                 "ElApache" \
                 "" \
                 "External/ElApache.pre.rules"

osbe_cxx_use_lib "Elements/HTML" \
                 "ElHTML" \
                 "." \
                 "build/lib" \
                 "ElHTML" \
                 "" \
                 "External/ElHTML.pre.rules"

osbe_cxx_use_lib "Elements/XML" \
                 "ElXML" \
                 "." \
                 "build/lib" \
                 "ElXML" \
                 "" \
                 "External/ElXML.pre.rules"

osbe_cxx_use_lib "Elements/LibXML" \
                 "ElLibXML" \
                 "." \
                 "build/lib" \
                 "ElLibXML" \
                 "" \
                 "External/ElLibXML.pre.rules"

osbe_cxx_use_lib "Elements/Geography" \
                 "ElGeography" \
                 "." \
                 "build/lib" \
                 "ElGeography" \
                 "" \
                 "External/ElGeography.pre.rules"

osbe_cxx_use_lib "Elements/Localization" \
                 "ElLocalization" \
                 "." \
                 "build/lib" \
                 "ElLocalization" \
                 "" \
                 "External/ElLocalization.pre.rules"

osbe_cxx_use_lib "Elements/Dictionary" \
                 "ElDictionary" \
                 "." \
                 "build/lib" \
                 "ElDictionary" \
                 "" \
                 "External/ElDictionary.pre.rules"

osbe_cxx_use_lib "Elements/CORBA/Corba" \
                 "ElCorba" \
                 "." \
                 "build/lib" \
                 "ElCorba" \
                 "" \
                 "External/ElCorba.pre.rules"

osbe_cxx_use_lib "Elements/Python" \
                 "ElPython" \
                 "." \
                 "build/lib" \
                 "ElPython" \
                 "" \
                 "External/ElPython.pre.rules"

osbe_cxx_use_lib "Elements/PythonInterceptor" \
                 "ElPythonInterceptor" \
                 "." \
                 "build/lib" \
                 "ElPythonInterceptor" \
                 "" \
                 "External/ElPythonInterceptor.pre.rules"

osbe_cxx_use_lib "Elements/PythonSandboxService" \
                 "ElPythonSandboxService" \
                 "." \
                 "build/lib" \
                 "ElPythonSandboxService" \
                 "" \
                 "External/ElPythonSandboxService.pre.rules"

osbe_cxx_use_lib "Elements/PSP" \
                 "ElPSP" \
                 "." \
                 "build/lib" \
                 "ElPSP" \
                 "" \
                 "External/ElPSP.pre.rules"

osbe_cxx_use_lib "Elements/TokyoCabinet" \
                 "ElTokyoCabinet" \
                 "." \
                 "build/lib" \
                 "ElTokyoCabinet" \
                 "" \
                 "External/ElTokyoCabinet.pre.rules"

m4_include([dir.ac])

OSBE_OUTPUT
