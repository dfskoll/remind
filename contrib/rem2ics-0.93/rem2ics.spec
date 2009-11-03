Name:           rem2ics
Version:        0.93
Release:        1%{?dist}
Summary:        Converts the output of "remind -s" into RFC2445 iCalendar format

Group:          Applications/Productivity
License:        GPLv2+
URL:            http://mark.atwood.name/code/rem2ics/
Source0:        http://mark.atwood.name/code/rem2ics/rem2ics-%{version}.tar.gz
Source1:        rem2ics-Makefile
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildArch:      noarch
BuildRequires:  perl


%description
rem2ics converts the output of "remind -s" into RFC2445 iCalendar format.
You may want to install remind if you install this package.


%prep
%setup -q -c
cp -a %SOURCE1 Makefile


%build
make  %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc
%{_bindir}/rem2ics
%{_mandir}/man1/rem2ics.1*


%changelog
* Tue Mar 25 2008 Till Maas <opensource till name> - 0.92-1
- initial spec for Fedora
