#ifdef HELP
#undef HELP

Macro-Arguments:

  HELP  - outputs this help information.

  DEBUG - disables inline function instantiation and code optimization.

  ARC_i386, ARC_x86_64 - mutually exclusive, ARC_i386 by default.
                         Specifies the platform binaries intended for.

  ORB_MICO, ORB_TAO, ORB_ORBacus - mutually exclusive, ORB_MICO by default.
                                   Specifies the ORB implementation to be used.

  ELEMENTS_SRC   - Specifies path to Elements sources directory.

  ELEMENTS_BUILD - Specifies path to Element build directory.

Usage Examples:

  cpp -DDEBUG -DARC_x86_64 -DORB_ORBacus -DELEMENTS_SRC=/home/vasya/projects/Elements -DELEMENTS_BUILD=/home/vasya/projects/Elements/build default.config.t > ./build/default.config

  cpp -DHELP default.config.t

#else

#ifdef DEBUG
# define OPTIMIZATION_FLAGS -fno-inline
#else
# define OPTIMIZATION_FLAGS -O3
#endif

#if defined(ARC_i386) && defined(ARC_x86_64)
# error ARC_i386 and ARC_x86_64 are mutually exclusive
#endif

#if !defined(ARC_i386) && !defined(ARC_x86_64)
# define ARC_i386
#endif

#ifdef ARC_i386
# define LIB_DIR lib
# define PLATFORM_FLAGS -m32 -DM32
#endif

#ifdef ARC_x86_64
# define LIB_DIR lib64
# define PLATFORM_FLAGS -m64 -DM64
#endif

#if !defined(ORB_MICO) && !defined(ORB_ORBacus) && !defined(ORB_TAO)
# define ORB_TAO
#endif

cxx.id = GCC
cxx.cxx = g++
cxx.cpp_flags = PLATFORM_FLAGS -DPIC -D_REENTRANT -D_GLIBCPP_USE_WCHAR_T -DPROTOTYPES -D__STDC_LIMIT_MACROS -std=c++0x
cxx.flags = PLATFORM_FLAGS -pthread -fPIC -Wall -Wno-deprecated-declarations -Wno-deprecated -g -ggdb OPTIMIZATION_FLAGS
cxx.obj.ext = .o

cxx.ld.flags = PLATFORM_FLAGS -pthread -Wall
cxx.ld.libs = 
cxx.so.ld = g++ -shared
cxx.so.primary.target.templ = lib%.so
cxx.so.secondary.target.templ = 
cxx.ex.ld = g++
cxx.ex.target.templ = %

cxx.xml.xsd.id = XSD
cxx.xml.xsd.dir = /usr
cxx.xml.xsd.translator = /usr/bin/xsd
cxx.xml.xsd.cppflags = -I/usr/include
cxx.xml.xsd.cxxflags = 
cxx.xml.xsd.ldflags = -L/usr/LIB_DIR
cxx.xml.xsd.libs = -lxerces-c
cxx.xml.xsd.translator_common_flags = 
cxx.xml.xsd.translator_tree_flags = --namespace-regex \"|^XMLSchema.xsd http://www.w3.org/2001/XMLSchema\$|xml_schema|\"
cxx.xml.xsd.translator_parser_flags = --namespace-regex \"|^XMLSchema.xsd http://www.w3.org/2001/XMLSchema\$|xml_schema_parser|\"
external.lib.xsd_xerces.root = /usr
external.lib.xsd_xerces.cxx.include = /usr/include
external.lib.xsd_xerces.cxx.lib = /usr/LIB_DIR
external.lib.xsd_xerces.cxx.so_files = xerces-c
external.lib.xsd_xerces.cxx.ar_files = 
external.lib.xsd_xerces.cxx.ld_flags = 

#ifdef ORB_MICO
# define ORB_ID MICO
cxx.corba.orb.id = MICO
cxx.corba.orb.dir = /opt/mico
cxx.corba.orb.idl = /opt/mico/bin/idl
cxx.corba.cppflags = -I/opt/mico/include
cxx.corba.cxxflags = 
cxx.corba.ldflags = -L/opt/mico/LIB_DIR
cxx.corba.libs = -lmico2.3.13
cxx.corba.skelsuffix = _s
cxx.corba.stubsuffix = 
cxx.corba.idlppflags = -I/opt/mico/include
cxx.corba.extraidlflags = 
cxx.corba.orb.mico.idl.libpath = /opt/mico/lib64 /opt/mico/lib
cxx.corba.orb.mico.binpath = /opt/mico/bin
external.lib.mico_openssl.root = /usr
external.lib.mico_openssl.cxx.include = 
external.lib.mico_openssl.cxx.lib = 
external.lib.mico_openssl.cxx.so_files = ssl crypto
external.lib.mico_openssl.cxx.ar_files = 
external.lib.mico_openssl.cxx.ld_flags = 
#endif

#ifdef ORB_ORBacus
# if defined(ORB_ID)
#   error ORB_MICO, ORB_TAO and ORB_ORBacus are mutually exclusive
# endif
# define ORB_ID ORBacus
cxx.corba.orb.id = ORBacus
cxx.corba.orb.dir = /usr
cxx.corba.orb.idl = /usr/bin/idl
cxx.corba.cppflags = -I/usr/include/OB -I/usr/include -I.
cxx.corba.cxxflags = 
cxx.corba.ldflags = -L/usr/LIB_DIR
cxx.corba.libs = -lOB -lOBNaming -lJTC -ldl
cxx.corba.skelsuffix = _s
cxx.corba.stubsuffix = 
cxx.corba.idlppflags = -I/usr/idl/OB -I/usr/idl
cxx.corba.extraidlflags = 
cxx.corba.orb.orbacus.idl.libpath = /usr/lib64 /usr/lib
#endif

#ifdef ORB_TAO
# if defined(ORB_ID)
#   error ORB_MICO, ORB_TAO and ORB_ORBacus are mutually exclusive
# endif
# define ORB_ID TAO
cxx.corba.orb.id = TAO
cxx.corba.orb.dir = /usr
cxx.corba.orb.idl = /usr/bin/tao_idl
cxx.corba.cppflags = -I/usr/include
cxx.corba.cxxflags = 
cxx.corba.ldflags = -L/usr/LIB_DIR
cxx.corba.libs = -lTAO -lTAO_AnyTypeCode -lTAO_CodecFactory -lTAO_DynamicAny -lTAO_EndpointPolicy -lTAO_FaultTolerance -lTAO_IORTable -lTAO_Messaging -lTAO_PI -lTAO_PI_Server -lTAO_PortableServer -lTAO_Security -lTAO_SSLIOP -lTAO_TC -lTAO_TC_IIOP -lTAO_Utils -lTAO_Valuetype -lACE -lACE_SSL
cxx.corba.skelsuffix = _s
cxx.corba.stubsuffix = 
cxx.corba.idlppflags = -I/usr/include
cxx.corba.extraidlflags = 
cxx.corba.orb.tao.idl.libpath = /usr/LIB_DIR
cxx.corba.orb.tao.ace_binpath = /usr/bin
external.lib.tao_openssl.root = /usr
external.lib.tao_openssl.cxx.include = 
external.lib.tao_openssl.cxx.lib = 
external.lib.tao_openssl.cxx.so_files = ssl crypto
external.lib.tao_openssl.cxx.ar_files = 
external.lib.tao_openssl.cxx.ld_flags = 
#endif

documentation.doxygen.path = doxygen

external.lib.python.root = /usr
external.lib.python.cxx.lib = /usr/LIB_DIR

external.lib.md5.root = /usr
external.lib.md5.cxx.include = /usr/include/openssl
external.lib.md5.cxx.lib = /usr/LIB_DIR
external.lib.md5.cxx.so_files = ssl
external.lib.md5.cxx.ar_files = 

external.lib.libxml.root = /usr
external.lib.libxml.cxx.include = /usr/include/libxml2
external.lib.libxml.cxx.lib = /usr/LIB_DIR
external.lib.libxml.cxx.so_files = xml2
external.lib.libxml.cxx.ar_files = 

external.lib.zlib.root = /usr
external.lib.zlib.cxx.include = /usr/include
external.lib.zlib.cxx.lib = /usr/LIB_DIR
external.lib.zlib.cxx.so_files = z
external.lib.zlib.cxx.ar_files = 

external.lib.uuid.root = /usr
external.lib.uuid.cxx.include = /usr/include
external.lib.uuid.cxx.lib = /usr/LIB_DIR
external.lib.uuid.cxx.so_files = uuid
external.lib.uuid.cxx.ar_files = 

external.lib.google.root = /usr
external.lib.google.cxx.include = /usr/include
external.lib.google.cxx.lib =
external.lib.google.cxx.so_files = 
external.lib.google.cxx.ar_files = 

external.lib.ace.root = /usr
external.lib.ace.cxx.include = /usr/include
external.lib.ace.cxx.lib = /usr/LIB_DIR
external.lib.ace.cxx.so_files = ACE
external.lib.ace.cxx.ar_files = 

external.lib.xerces.root = /usr
external.lib.xerces.cxx.include = /usr/include
external.lib.xerces.cxx.lib = /usr/LIB_DIR
external.lib.xerces.cxx.so_files = xerces-c
external.lib.xerces.cxx.ar_files = 

external.lib.graphicsmagick.root = /usr
external.lib.imagemagick.root = /usr
external.lib.imagemagick.cxx.include = /usr/include/ImageMagick
external.lib.imagemagick.cxx.lib = /usr/LIB_DIR
external.lib.imagemagick.cxx.so_files = Magick++
external.lib.imagemagick.cxx.ar_files = 

external.lib.apache.root = /usr
external.lib.apache.cxx.include = /usr/include /usr/include/apr-1
external.lib.apache.cxx.lib = /usr/LIB_DIR
external.lib.apache.cxx.so_files = 
external.lib.apache.cxx.ar_files = 

external.lib.tokyocabinet.root = /usr
external.lib.tokyocabinet.cxx.include = /usr/include
external.lib.tokyocabinet.cxx.lib = /usr/LIB_DIR
external.lib.tokyocabinet.cxx.so_files = tokyocabinet
external.lib.tokyocabinet.cxx.ar_files = 
external.lib.tokyocabinet.cxx.ld_flags = 

external.lib.mysql.root = /usr
external.lib.mysql.cxx.include = /usr/include
external.lib.mysql.cxx.lib = /usr/LIB_DIR/mysql
external.lib.mysql.cxx.so_files = mysqlclient_r z
external.lib.mysql.cxx.ar_files = 

external.lib.elements_basic.root = ELEMENTS_SRC
external.lib.elements_basic.cxx.include = ELEMENTS_SRC/.
external.lib.elements_basic.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_basic.cxx.so_files = Elements
external.lib.elements_basic.cxx.ar_files = 

external.lib.elements_mysql.root = ELEMENTS_SRC
external.lib.elements_mysql.cxx.include = ELEMENTS_SRC/.
external.lib.elements_mysql.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_mysql.cxx.so_files = ElMySQL
external.lib.elements_mysql.cxx.ar_files = 

external.lib.elements_net.root = ELEMENTS_SRC
external.lib.elements_net.cxx.include = ELEMENTS_SRC/.
external.lib.elements_net.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_net.cxx.so_files = ElNet
external.lib.elements_net.cxx.ar_files = 

external.lib.elements_apache.root = ELEMENTS_SRC
external.lib.elements_apache.cxx.include = ELEMENTS_SRC/.
external.lib.elements_apache.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_apache.cxx.so_files = ElApache
external.lib.elements_apache.cxx.ar_files = 

external.lib.elements_html.root = ELEMENTS_SRC
external.lib.elements_html.cxx.include = ELEMENTS_SRC/.
external.lib.elements_html.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_html.cxx.so_files = ElHTML
external.lib.elements_html.cxx.ar_files = 

external.lib.elements_xml.root = ELEMENTS_SRC
external.lib.elements_xml.cxx.include = ELEMENTS_SRC/.
external.lib.elements_xml.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_xml.cxx.so_files = ElXML
external.lib.elements_xml.cxx.ar_files = 

external.lib.elements_libxml.root = ELEMENTS_SRC
external.lib.elements_libxml.cxx.include = ELEMENTS_SRC/.
external.lib.elements_libxml.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_libxml.cxx.so_files = ElLibXML
external.lib.elements_libxml.cxx.ar_files = 

external.lib.elements_geography.root = ELEMENTS_SRC
external.lib.elements_geography.cxx.include = ELEMENTS_SRC/.
external.lib.elements_geography.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_geography.cxx.so_files = ElGeography
external.lib.elements_geography.cxx.ar_files = 

external.lib.elements_localization.root = ELEMENTS_SRC
external.lib.elements_localization.cxx.include = ELEMENTS_SRC/.
external.lib.elements_localization.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_localization.cxx.so_files = ElLocalization
external.lib.elements_localization.cxx.ar_files = 

external.lib.elements_dictionary.root = ELEMENTS_SRC
external.lib.elements_dictionary.cxx.include = ELEMENTS_SRC/.
external.lib.elements_dictionary.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_dictionary.cxx.so_files = ElDictionary
external.lib.elements_dictionary.cxx.ar_files = 

external.lib.elements_corba_corba.root = ELEMENTS_SRC
external.lib.elements_corba_corba.cxx.include = ELEMENTS_SRC/. ELEMENTS_BUILD ELEMENTS_BUILD/El/CORBA
external.lib.elements_corba_corba.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_corba_corba.cxx.so_files = ElCorba
external.lib.elements_corba_corba.cxx.ar_files = 

external.lib.elements_python.root = ELEMENTS_SRC
external.lib.elements_python.cxx.include = ELEMENTS_SRC/.
external.lib.elements_python.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_python.cxx.so_files = ElPython
external.lib.elements_python.cxx.ar_files = 

external.lib.elements_pythoninterceptor.root = ELEMENTS_SRC
external.lib.elements_pythoninterceptor.cxx.include = ELEMENTS_SRC/.
external.lib.elements_pythoninterceptor.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_pythoninterceptor.cxx.so_files = ElPythonInterceptor
external.lib.elements_pythoninterceptor.cxx.ar_files = 

external.lib.elements_pythonsandboxservice.root = ELEMENTS_SRC
external.lib.elements_pythonsandboxservice.cxx.include = ELEMENTS_SRC/.
external.lib.elements_pythonsandboxservice.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_pythonsandboxservice.cxx.so_files = ElPythonSandboxService
external.lib.elements_pythonsandboxservice.cxx.ar_files = 

external.lib.elements_psp.root = ELEMENTS_SRC
external.lib.elements_psp.cxx.include = ELEMENTS_SRC/.
external.lib.elements_psp.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_psp.cxx.so_files = ElPSP
external.lib.elements_psp.cxx.ar_files = 

external.lib.elements_tokyocabinet.root = ELEMENTS_SRC
external.lib.elements_tokyocabinet.cxx.include = ELEMENTS_SRC/.
external.lib.elements_tokyocabinet.cxx.lib = ELEMENTS_BUILD/lib
external.lib.elements_tokyocabinet.cxx.so_files = ElTokyoCabinet
external.lib.elements_tokyocabinet.cxx.ar_files = 

#endif // HELP
