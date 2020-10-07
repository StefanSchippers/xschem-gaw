Name:           gaw3
Version:        20170716
Release:        1%{?dist}
Summary:        Gtk Analog Wave viewer

License:        GPL
URL:            http://www.rvq.fr/linux/gaw.php
Source0:        gaw3-20170716.tar.gz

#BuildRequires:  
#Requires:       

%description


%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
%make_install


%files
%doc
%{_bindir}/*
/usr/share



%changelog
