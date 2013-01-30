## @file
# implementation of DoxyGen::PerlFilter.
#
# @copy 2002, Bart Schuller <cpan@smop.org>
# $Id: PerlFilter.pm,v 1.1.1.1 2006/01/30 08:51:02 aeby Exp $


## @class
# Filter from perl syntax API docs to Doxygen-compatible syntax.
# This class is meant to be used as a filter for the
# <a href="http://www.doxygen.org/">Doxygen</a> documentation tool.
package DoxyGen::PerlFilter;

use warnings;
use strict;
use base qw(DoxyGen::Filter);

## @method void filter($infh)
# Do the filtering.
# @param infh input filehandle, normally STDIN
sub filter {
    my($self, $infile) = @_;
    open(my $infh, $infile);
    my $current_class;
    while (<$infh>) {
        if (/^##\s*\@(\S+)\s*(.*)/) {
            my($command, $args) = ($1, $2);
            my @more;
            while (<$infh>) {
                if (/^#\s?(.+)/s) {
                    push @more, $1;
                } else {
                    last;
                }
            }
            if ($command eq 'file') {
                $args ||= $infile;
                $self->start("\@$command $args");
                $self->more(@more);
                $self->end;
            } elsif ($command eq 'class') {
                unless ($args) {
                    ($args) = /package\s(.*);/;
                    $args =~ s/::/__/g if $args;
                }
                if ($current_class) {
                    $self->flush;
                    $self->print("}\n");
                }
                $current_class = $args;

                my(@current_isa, @current_include);
                while (<$infh>) {
                    if (/^\s*(?:use base|\@ISA\s*=)\s+(.+);/) {
                        @current_isa = eval $1;
                    } elsif (/^use\s+([\w:]+)/) {
                        my $inc = $1;
                        $inc =~ s/::/\//g;
                        push @current_include, $inc;
                    } elsif (/^#/) {
                        last;
                    }
                }

                $self->print("#include \"$_.pm\"\n") foreach @current_include;
                
                $self->start("\@$command $args")->more(@more);
                $self->print("\@nosubgrouping")->end;
                $self->print("class $args");

                @current_isa = map { $_ =~ s/::/__/g; $_ } @current_isa;
                if (@current_isa) {
                    $self->print(":",
                        join(", ", map {"public $_"} @current_isa) );
                }
                $self->print(" {\npublic:\n");
            } elsif ($command  eq 'cmethod') {
                unless ($args) {
                    ($args) = /sub\s+(.*)\{/;
                    $args = "retval $args(\@args)";
                }
                $args = $self->munge_parameters($args);
                $self->push($self->protection($args).' Class Methods');
                $self->start("\@fn $args")->more(@more)->end;
                $self->print($args, ";\n");
                $self->pop;
            } elsif ($command  eq 'fn') {
                unless ($args) {
                    ($args) = /sub\s+(.*)\{/;
                    $args = "retval $args(\@args)";
                }
                $args = $self->munge_parameters($args);
                $self->push($self->protection($args).' Functions');
                $self->start("\@fn $args")->more(@more)->end;
                $self->print($args, ";\n");
                $self->pop;
            } elsif ($command  eq 'method') {
                unless ($args) {
                    ($args) = /sub\s+(.*)\{/;
                    $args = "retval $args(\@args)";
                }
                $args = $self->munge_parameters($args);
                $self->push($self->protection($args).' Object Methods');
                $self->start("\@fn $args")->more(@more)->end;
                $self->print($args, ";\n");
                $self->pop;
            } elsif ($command  eq 'enum') {
                $self->start("\@$command $args");
                $self->more(@more);
                $self->end;
                $self->print("$command $args;\n");
            } else {
                $self->start("\@$command $args");
                $self->more(@more);
                $self->end;
            }
            # We ate a line when we got the rest of the comment lines
            redo if defined $_;
        } elsif (/^use\s+([\w:]+)/) {
            my $inc = $1;
            $inc =~ s/::/\//g;
            $self->print("#include \"$inc.pm\"\n");
        }
    }
    if ($current_class) {
        $self->flush;
        $self->print("}\n");
    }
}


## @method munge_parameters($args)
# Munge the argument list. Because DoxyGen does not seem to handle $, @ and %
# as argument types properly, we replace them with full length strings.
#
# @param args String specifying anything after a directive
# @return Processed string.
sub munge_parameters {
    my ($this, $args) = @_;

    $args =~ s/\$\@/scalar_or_list /g;
    $args =~ s/\@\$/scalar_or_list /g;
    $args =~ s/\$/scalar /g;
    $args =~ s/\@/list /g;
    $args =~ s/\%/hash /g;

#    my ($ret, $remainder) = ($args =~ /^\s*(\S+)(.+)/);
#    if ($ret) {
#        if ($ret eq '$') {
#            $ret = 'scalar';
#        } elsif ($ret eq '@') {
#            $ret = 'list';
#        } elsif ($ret eq '$@') {
#            $ret = 'scalar_or_list';
#        } elsif ($ret eq '@$') {
#            $ret = 'list_or_scalar';
#        }
#
#        $args = "$ret$remainder";
#    }

    return $args;
}


1;
