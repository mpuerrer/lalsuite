%define name ligo-common
%define version 1.0.2
%define unmangled_version 1.0.2
%define release 1

Summary: Empty LIGO modules
Name: %{name}
Version: %{version}
Release: %{release}%{?dist}
Source0: %{name}-%{unmangled_version}.tar.gz
License: GPL
Group: Development/Libraries
Prefix: %{_prefix}
BuildArch: noarch
Requires: python
BuildRequires: python python-setuptools
Vendor: Tanner Prestegard <tanner.prestegard@ligo.org>

%description
Empty module placeholder for other LIGO modules

%prep
%setup -n %{name}-%{unmangled_version}

%build
python setup.py build

%install
python setup.py install --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT --record=INSTALLED_FILES

%clean
rm -rf $RPM_BUILD_ROOT

%files -f INSTALLED_FILES
%defattr(-,root,root)
