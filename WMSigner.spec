Summary: WMSigner module for Webmoney signing data
Name: WMSigner
Version: 1.0
Release: 1
Group: Applications/Tools
Copyright: BSD
Source: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-root
Requires: gcc-c++
%description
WMSigner module for Webmoney signing data

%prep
%setup -q

%build
#./configure --prefix=/usr/local/bin
make

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
make install prefix=%{buildroot}

%post
cat << EOF
You need to configure edit WMSigner.ini
EOF

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%attr(0755, root, bin) /usr/local/bin/WMSigner
%attr(0644, root, bin) /usr/local/bin/WMSigner.ini
%attr(0644, root, bin) /usr/local/man/man1/WMSigner.1
