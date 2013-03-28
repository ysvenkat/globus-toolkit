Name:		myproxy-oauth
%global _name %(tr - _ <<< %{name})
Version:	0.0
Release:	1%{?dist}
Summary:	MyProxy OAuth Delegation Serice

Group:		System Environment/Libraries
License:	ASL 2.0
URL:		http://www.globus.org/
Source:		http://www.globus.org/ftppub/gt5/5.2/stable/packages/src/%{_name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	python
BuildArch:      noarch

Requires(pre): shadow-utils
Requires:	mod_wsgi
Requires:	mod_ssl
Requires:       python-httplib2
Requires:	pyOpenSSL
Requires:       python-crypto >= 2.2
Requires:	python-flask >= 0.7
Requires:	python-sqlalchemy
Requires:	httpd
%if 0%{?rhel} == 06
Requires:       python-jinja2-26
%endif

%description
The Globus Toolkit is an open source software toolkit used for building Grid
systems and applications. It is being developed by the Globus Alliance and
many others all over the world. A growing number of projects and companies are
using the Globus Toolkit to unlock the potential of grids for their cause.

The %{name} package contains:
MyProxy OAuth Delegation Service

%prep
%setup -q -n %{_name}-%{version}

%build
:

%install
rm -rf $RPM_BUILD_ROOT
python setup.py install \
    --install-lib /usr/share/%{name} \
    --install-scripts /usr/share/%{name} \
    --install-data %{_docdir}/%{name} \
    --root $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{_docdir}/%{name}
cp README.md $RPM_BUILD_ROOT%{_docdir}/%{name}/README.txt
mkdir -p $RPM_BUILD_ROOT/etc/httpd/conf.d
%if 0%{?fedora} >= 18
cp $RPM_BUILD_ROOT%{_docdir}/%{name}/apache/myproxy-oauth-2.4 \
   $RPM_BUILD_ROOT/etc/httpd/conf.d/wsgi-myproxy-oauth.conf 
%else
cp $RPM_BUILD_ROOT%{_docdir}/%{name}/apache/myproxy-oauth \
   $RPM_BUILD_ROOT/etc/httpd/conf.d/wsgi-myproxy-oauth.conf 
%endif

mkdir -p "$RPM_BUILD_ROOT/var/lib/myproxy-oauth"

%pre
getent group myproxyoauth >/dev/null || groupadd -r myproxyoauth
getent passwd myproxyoauth >/dev/null || \
    useradd -r -g myproxyoauth -d /usr/share/myproxy-oauth -s /sbin/nologin \
        -c "MyProxy Oauth Daemon" myproxyoauth
        exit 0

%post
service httpd condrestart
%postun
service httpd condrestart

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc %{_docdir}/%{name}/README.txt
%doc %{_docdir}/%{name}/apache/*
%config(noreplace) /etc/httpd/conf.d/wsgi-myproxy-oauth.conf
%dir %attr(0700,myproxyoauth,myproxyoauth) /var/lib/myproxy-oauth
/usr/share/%{name}

%changelog
* Wed Mar 27 2013 Globus Toolkit <support@globus.org> - 0.0-1
- Initial packaging