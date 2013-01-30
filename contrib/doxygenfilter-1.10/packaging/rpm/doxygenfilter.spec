#
# spec file for package DoxyGenFilter
#
# Copyright (c) 2006 Thomas Aeby, Switzerland
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

Name:         doxygenfilter
Version:      1.00
Release:      4
License:	GPL, 2002 Bart Schuller, 2006 Thomas Aeby
Group:        util
Url:		http://doxygenfilter.bigsister.ch/
Provides:	doxygenfilter
Summary:      Doxygen Input Filter enabling Perl support
Source:		%{name}-%{version}.tar.bz2
%define		MyRoot	 	%{_tmppath}/%{name}-%{version}-build
BuildRoot:    %{MyRoot}
BuildArchitectures: noarch
AutoReq: 0

%description
DoxyGen-Filter is a Perl script and accompanying Perl modules that
can be used as INPUT_FILTER for doxygen. It converts Perl files
in a way that doxygen will be able to generate code documentation
from Doxygen-Filter's output.

%prep
%setup

%build
perl Makefile.PL INSTALLDIRS=vendor
make

%install
mkdir -p $RPM_BUILD_ROOT/usr/lib
make install PREFIX=$RPM_BUILD_ROOT/usr
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/doxygenfilter
cp CHANGES $RPM_BUILD_ROOT/usr/share/doc/doxygenfilter
cp README $RPM_BUILD_ROOT/usr/share/doc/doxygenfilter
tar cf - examples | tar xf - -C $RPM_BUILD_ROOT/usr/share/doc/doxygenfilter


%files
%docdir /usr/share/doc/doxygenfilter
/usr/bin/doxygenfilter
/usr/share
/usr/lib

%post
# Depending on the distribution the RPM was built and is installed,
# the vendor tree may be found under different directories. Try to
# add symbolic links ...

rpmtree=""
for dir in /usr/share/perl5 /usr/lib/perl5 /usr/lib/perl5/vendor_perl /usr/share/perl6 /usr/lib/perl6 /usr/lib/perl6/vendor_perl; do
    [ -d $dir/DoxyGen -a ! -h $dir/Doxygen ] && rpmtree="$dir/DoxyGen"
done
if [ "$rpmtree" = "" ]; then
    echo "Strange - I do not know where DoxyGenFilter has been installed" >&2
    exit 0
fi
vendortree=`perl -e 'use Config; print $Config{"vendorlibexp"}'`
if [ "$vendortree" = "" -o ! -d "$vendortree" ]; then
    echo "Cannot find out where your system requires Perl libraries to be installed" >&2
    echo "This library was installed in $rpmtree, where Perl might actually not find it" >&2
    exit 0
fi

if [ -h "$vendortree/DoxyGen" ]; then
    rm "$vendortree/DoxyGen"
fi
ln -s $rpmtree $vendortree/DoxyGen
