%define DIRNAME admin
%define SPECNAME smartmet-plugin-%{DIRNAME}
Summary: BrainStorm admin plugin
Name: %{SPECNAME}
Version: 20.2.27
Release: 1%{?dist}.fmi
License: MIT
Group: SmartMet/Plugins
URL: https://github.com/fmidev/smartmet-plugin-admin
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: boost-devel
BuildRequires: libconfig-devel
BuildRequires: smartmet-library-macgyver-devel >= 20.2.27
BuildRequires: smartmet-library-spine-devel >= 20.2.26
BuildRequires: smartmet-engine-contour-devel
BuildRequires: smartmet-engine-geonames-devel >= 19.12.5
BuildRequires: smartmet-engine-sputnik-devel >= 19.9.26
BuildRequires: smartmet-engine-querydata-devel >= 20.1.30
BuildRequires: smartmet-engine-observation-devel >= 20.2.20
Requires: smartmet-library-macgyver >= 20.2.27
Requires: libconfig
Requires: smartmet-server >= 20.2.13
Requires: smartmet-library-spine >= 20.2.26
Requires: smartmet-engine-geonames >= 19.12.5
Requires: smartmet-engine-querydata >= 20.1.30
Requires: smartmet-engine-sputnik >= 19.9.26
Provides: %{SPECNAME}
Obsoletes: smartmet-brainstorm-admin < 16.11.1
Obsoletes: smartmet-brainstorm-admin-debuginfo < 16.11.1

%description
SmartMet admin plugin

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{SPECNAME}
 
%build -q -n %{SPECNAME}
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,0775)
%{_datadir}/smartmet/plugins/%{DIRNAME}.so
%defattr(0664,root,root,0775)

%changelog
* Thu Feb 27 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.27-1.fmi
- The content handler is now private, and thus not visible to any frontends

* Mon Feb 10 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.10-1.fmi
- Added sputnik RPM dependency

* Thu Sep 26 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.26-1.fmi
- Added support for ASAN & TSAN builds

* Wed Aug 14 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.8.14-1.fmi
- Rewrote request log handler to use the new log objects

* Thu Apr 25 2019 Andris Pavenis <andris.pavenis@fmi.fi> - 19.4.25-1.fmi
- Use SmartMet::Spine::HTTP::Authentication instead of own implementation

* Thu Mar 21 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.3.21-1.fmi
- Added possibility to list producers

* Thu Feb 28 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.2.28-1.fmi
- Added client IP to active requests report

* Thu Feb 14 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.2.14-1.fmi
- Added client IP to exception reports

* Thu Dec 13 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.12.13-2.fmi
- Removed explicit dependency on Sputnik-engine, requests will print error if not available

* Thu Dec 13 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.12.13-1.fmi
- Added duration option for continue/pause requests

* Tue Dec  4 2018 Pertti Kinnia <pertti.kinnia@fmi.fi> - 18.12.4-1.fmi
- Repackaged since Spine::Table size changed

* Thu Nov  8 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.11.8-1.fmi
- Added what=pause, what=pause&time=XXX and what=continue requests

* Thu Aug 30 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.30-1.fmi
- Silenced CodeChecker warnings

* Mon Aug 27 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.27-1.fmi
- Silenced CodeChecker warnings

* Sun Aug 26 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.26-1.fmi
- Silenced CodeChecker warnings

* Mon Aug  6 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.6-1.fmi
- Recompiled since querydata-engine cache statistics API changed

* Fri Jul 27 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.7.27-1.fmi
- Avoid iostreams while formatting times to avoid global locale locks

* Wed Jul 25 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.7.25-1.fmi
- Prefer nullptr over NULL

* Sun May 27 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.5.27-1.fmi
- Changed to use the latest spine API for logged requests

* Wed May  9 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.4.9-1.fmi
- Added reporting of active requests with what=activerequests

* Sat Apr  7 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.4.7-1.fmi
- Upgrade to boost 1.66

* Thu Mar 22 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.3.22-1.fmi
- Added "serviceinfo" request

* Tue Mar 20 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.3.20-1.fmi
- Full recompile of all server plugins

* Mon Aug 28 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.28-1.fmi
- Upgrade to boost 1.65

* Tue Apr 11 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.4.10-1.fmi
- Enable CORS by allowing Access-Control-Allow-Origin for all hosts

* Sat Apr  8 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.4.8-1.fmi
- Simplified error reporting

* Wed Mar 15 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.3.15-1.fmi
- Recompiled since Spine::Exception changed

* Sat Feb 11 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.11-1.fmi
- Repackaged due to newbase API change

* Wed Jan  4 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.4-1.fmi
- Changed to use renamed SmartMet base libraries

* Wed Nov 30 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.30-1.fmi
- No installation for configuration

* Tue Nov  1 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.1-1.fmi
- Namespaces refactored

* Tue Sep 13 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.9.13-1.fmi
- References to hints-cache removed, because hints-cache no more exists in Contour-engine

* Tue Sep  6 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.9.6-1.fmi
- New exception handler

* Tue Aug 30 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.8.30-1.fmi
- Base class API change

* Mon Aug 15 2016 Markku Koskela <markku.koskela@fmi.fi> - 16.8.15-1.fmi
- The init(),shutdown() and requestHandler() methods are now protected methods
- The requestHandler() method is called from the callRequestHandler() method

* Wed Jun 29 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.29-1.fmi
- QEngine API changed

* Tue Jun 14 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.14-1.fmi
- Full recompile

* Thu Jun  2 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.2-1.fmi
- Full recompile

* Wed Jun  1 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.1-1.fmi
- Added graceful shutdown

* Wed May 11 2016 Tuomo Lauri <tuomo.lauri@fmi.fi> - 16.5.11-2.fmi
- Added Access Control header to cache size query

* Wed May 11 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.5.11-1.fmi
- Added cache size reporting

* Mon Jan 18 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.18-1.fmi
- newbase API changed, full recompile

* Wed Nov 18 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.11.18-1.fmi
- SmartMetPlugin now receives a const HTTP Request

* Mon Oct 26 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.10.26-1.fmi
- Added proper debuginfo packaging

* Mon Aug 24 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.24-1.fmi
- Recompiled due to Convenience.h API changes

* Tue Aug 18 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.18-1.fmi
- Recompile forced by brainstorm API changes

* Mon Aug 17 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.17-1.fmi
- Use -fno-omit-frame-pointer to improve perf use

* Thu Apr  9 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.9-1.fmi
- newbase API changed

* Wed Apr  8 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.8-1.fmi
- Dynamic linking of smartmet libraries into use

* Thu Dec 18 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.12.18-1.fmi
- Recompiled due to spine API changes

* Mon Sep  8 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.9.8-1.fmi
- Recompiled due to geonengine API changes

* Mon Jun 30 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.6.30-1.fmi
- Recompiled due to spine API changes

* Wed May 14 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.14-1.fmi
- Use shared macgyver and locus libraries

* Mon Apr 28 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.4.28-1.fmi
- Full recompile due to large changes in spine etc APIs

* Tue Nov  5 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.11.5-1.fmi
- Major release

* Wed Oct  9 2013 Tuomo Lauri <tuomo.lauri@fmi.fi> - 13.10.9-1.fmi
- Now conforming with the new Reactor initialization API

* Fri Sep 6  2013 Tuomo Lauri    <tuomo.lauri@fmi.fi>    - 13.9.6-1.fmi
- Recompiled due Spine changes

* Mon Aug 12 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.8.12-1.fmi
- Sputnik protobuf message changed

* Tue Jul 23 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.7.23-1.fmi
- Recompiled due to thread safety fixes in newbase & macgyver

* Wed Jul  3 2013 mheiskan <mika.heiskanen@fmi.fi> - 13.7.3-1.fmi
- Update to boost 1.54

* Mon Jun  3 2013 lauri    <tuomo.lauri@fmi.fi>   - 13.6.3-1.fmi
- Built against the new Spine

* Mon Apr 22 2013 mheiskan <mika.heiskanen@fi.fi> - 13.4.22-1.fmi
- Brainstorm API changed

* Fri Apr 12 2013 lauri <tuomo.lauri@fmi.fi>    - 13.4.12-1.fmi
- Rebuild due to changes in Spine

* Tue Mar 26 2013 mheiskan <mika.heiskanen@fmi.fi> - 13.3.26-1.fmi
- Display possible GeoEngine reload error messages in the response

* Wed Feb  6 2013 lauri    <tuomo.lauri@fmi.fi>    - 13.2.6-1.fmi
- Built against new Spine and Server

* Wed Nov  7 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.11.7-1.fmi
- Upgrade to boost 1.52
- Upgrade to refactored spine library

* Fri Oct 19 2012 lauri    <tuomo.lauri@fmi.fi>    - 12.10.19-1.el6.fmi
- GeoEngine cache reporting moved to Ilmanet Brainstorm plugin

* Fri Oct 12 2012 lauri    <tuomo.lauri@mfmi.fi>   - 12.10.12-2.el6.fmi
- Added last minute request tracking

* Fri Oct 12 2012 lauri    <tuomo.lauri@mfmi.fi>   - 12.10.12-1.el6.fmi
- Added nicer looking Geonames cache reporting

* Mon Oct  8 2012 mheiskan <mika.heiskanen@fmi.fi> - 18.10.8-1.el6.fmi
- Recompiled to get html table formatting

* Thu Aug 16 2012 lauri    <tuomo.lauri@fmi.fi>    - 12.8.16-2.el6.fmi
- Fixed Qengine status query html-tags

* Thu Aug 16 2012 lauri    <tuomo.lauri@fmi.fi>    - 12.8.16-1.el6.fmi
- Added more info to service log reporting

* Wed Aug 15 2012 lauri    <tuomo.lauri@fmi.fi>    - 12.8.15-1.el6.fmi
- Added access to logging functionality

* Thu Aug  9 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.8.9-3.el6.fmi
- Fixed backends query

* Thu Aug  9 2012 lauri    <tuomo.lauri@fmi.fi>    - 12.8.9-2.el6.fmi
- Removed duplicate MIME header generation

* Thu Aug  9 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.8.9-1.el6.fmi
- Added backends query

* Thu Aug  9 2012 lauri    <tuomo.lauri@fmi.fi>    - 12.8.9-1.el6.fmi
- Added QEngine contents query

* Mon Aug  6 2012 lauri    <tuomo.lauri@fmi.fi>    - 12.8.6-1.el6.fmi
- Added basic authentication

* Thu Jul 26 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.26-1.el6.fmi
- geoengine API changed

* Wed Jul 25 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.25-3.el6.fmi
- Print output in UTF-8

* Wed Jul 25 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.25-2.el6.fmi
- Added dependency on macgyver library

* Wed Jul 25 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.25-1.el6.fmi
- Admin plugin now adds the necessary HTML tags to the engine responses

* Tue Jul 24 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.24-1.el6.fmi
- Added what=geonames query

* Mon Jul 23 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.23-1.el6.fmi
- The plugin now uses 5xx error headers for error situations so that Ilmanet can detect them

* Thu Jul 19 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.19-1.el6.fmi
- Renamed Status plugin to Admin
- Added reloading of GeoEngine

* Thu Jul  5 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.5-1.el6.fmi
- New plugin created by refactoring Sputnik engine
