use 5.012003;
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    NAME              => 'DaZeus',
    VERSION_FROM      => 'lib/DaZeus.pm', # finds $VERSION
    PREREQ_PM         => {
      JSON => '2.53',
    },
    ($] >= 5.005 ?     ## Add these new keywords supported since 5.005
      (ABSTRACT_FROM  => 'lib/DaZeus.pm', # retrieve abstract from module
       AUTHOR         => 'Sjors Gielen <dazeus@sjorsgielen.nl>') : ()),
);
