use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'		=> 'DoxyGen::Filter',
    'VERSION'	        => '1.0',
    'PREREQ_PM'		=> {'HTML::Entities' => 0},
    'ABSTRACT'          => 'Use Doxygen with Perl',
    'AUTHOR'            => 'Bart Schuller <cpan@smop.org>',
    'EXE_FILES'         => ['bin/doxygenfilter'],
);
