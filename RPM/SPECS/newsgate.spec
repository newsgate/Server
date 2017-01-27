%if %{?__target:1}%{!?__target:0}
%define _target_cpu %{__target}
%endif

%ifarch i386 i486 i586 i686
%define optflags %{__global_cflags} -m32 -march=%{_target_cpu} -mtune=generic -fasynchronous-unwind-tables
%define _archfix i386
%else
%define _archfix %_arch
%endif

%define __product               NewsGate
%define __inst_root             /opt/%{__product}
%define __osbe_build_dir        .

Name:    newsgate
Version: 1.7.4.0
Release: ssv1%{?dist}
Summary: News search WEB service
License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)
Group:   System Environment/Daemons
BuildRoot: %{_tmppath}/%{name}-buildroot
Source0: %name-%version.tar.gz

BuildRequires: make OpenSBE-defs gcc-c++ ace-tao-devel >= 5.6.5.23 google-sparsehash python-devel xerces-c-devel >= 3.1.1-ssv5.el5 GeoIP httpd-devel xsd ImageMagick-c++-devel >= 6.6.2 tokyocabinet-devel libidn-devel e2fsprogs-devel libxml2-devel >= 2.7.8-7 GeoIP-devel >= 1.4.8 libuuid-devel
Requires: httpd mod_ssl nc xerces-c >= 3.1.1-ssv5.el6 ace-tao >= 5.6.5.23 libxml2 >= 2.7.8-7 GeoLite-City

%if "%{?dist}" == ".el6"
BuildRequires : mysql-devel >= 5.5.30
Requires: mysql-server >= 5.5.30
%endif

%if "%{?dist}" == ".el7"
BuildRequires : mariadb-devel >= 5.5.37
Requires: mariadb-server >= 5.5.37
%endif

%description
News search WEB service

%package devel
Summary: Header files for developing NewsGate's plugins
Group:   Development/Libraries/C and C++

%description devel
Header files for developing NewsGate's plugins

%prep
%setup -q -n %name-%{version}

%build
pushd Elements

cpp -DARC_%{_archfix} default.config.t > default.config
osbe
./configure --enable-no-questions
%__make %_smp_mflags

popd
pushd Server

cpp -DARC_%{_archfix} -DELEMENTS_SRC=%_builddir/%name-%version/Elements \
                      -DELEMENTS_BUILD=%_builddir/%name-%version/Elements default.config.t > default.config
osbe
./configure --enable-no-questions
%__make %_smp_mflags

%install
rm -rf %buildroot
pushd Elements
make install destdir=%buildroot prefix=/opt/%{__product}
popd
pushd Server
make install destdir=%buildroot prefix=/opt/%{__product}
rm -rf %buildroot/opt/%{__product}/include/El

%pre
%_sbindir/groupadd -r -f -g 777 newsgate &>/dev/null ||:
%_sbindir/useradd  -m -c 'newsgate operation account' -u 777 -g newsgate newsgate &>/dev/null ||:

%files
%defattr(-, root, root)
%doc Server/COPYRIGHT Server/CHANGELOG Server/README.md Server/LICENSE
%dir %__inst_root
%dir %__inst_root/bin
%__inst_root/bin/*
%dir %__inst_root/etc
%__inst_root/etc/*
%dir %__inst_root/lib
%__inst_root/lib/*
%dir %__inst_root/www
%__inst_root/www/*
%dir %__inst_root/xsd
%__inst_root/xsd/*

%files devel
%defattr(-, root, root)
%dir %__inst_root/include
%__inst_root/include/*

%changelog
