## @file
# implementation of DoxyGen::SQLFilter.
#
# @copy 2002, Bart Schuller <cpan@smop.org>
# $Id: SQLFilter.pm,v 1.1.1.1 2006/01/30 08:51:02 aeby Exp $


## @class
# Filter from SQL syntax API docs to Doxygen-compatible syntax.
# This class is meant to be used as a filter for the
# <a href="http://www.doxygen.org/">Doxygen</a> documentation tool.
package DoxyGen::SQLFilter;

use warnings;
use strict;
use base qw(DoxyGen::Filter);

## @method void filter($infh)
# do the filtering.
# @param infh input filehandle, normally STDIN
sub filter {
    my($self, $infile) = @_;
    open(my $infh, $infile);
    my $current_class;
    while (<$infh>) {
        if (/\/\*\*\s*\@(\S+)\s*(.*)/) {
            my($command, $args) = ($1, $2);
            my @more;
            while (<$infh>) {
                if (/\*\//) {
                    # We expect to be on the line after a comment.
                    $_ = <$infh>;
                    last;
                } else {
                    /\s*(\*\s)?(.+)/s;
                    push @more, $2;
                }
            }
            if ($command eq 'file') {
                $args ||= $infile;
                $self->start("\@$command $args");
                $self->more(@more);
                $self->end;
            } elsif ($command eq 'class') {
                unless ($args) {
                    ($args) = /package\s(\S*)/;
                }
                if ($current_class) {
                    $self->flush;
                    $self->print("}\n");
                }
                $current_class = $args;
                $self->start("\@$command $args")->more(@more);
                $self->print("\@nosubgrouping")->end;

                $self->print("class $args");
                $self->print(" {\npublic:\n");
            } elsif ($command  eq 'fn') {
                unless ($args) {
                    ($args) = /function\s+(.*)\b/;
                    $args = "retval $args(\@args)";
                }
                $self->push($self->protection($args).' Functions');
                $self->start("\@fn $args")->more(@more)->end;
                $self->print($args, ";\n");
                $self->pop;
            } else {
                $self->start("\@$command $args");
                $self->more(@more);
                $self->end;
            }
            # We ate a line when we got the rest of the comment lines
            redo if defined $_;
        } elsif (/\@(\w+)/) {
            my $inc = $1;
            $self->print("#include \"$inc.sql\"\n");
        }
    }
    if ($current_class) {
        $self->flush;
        $self->print("}\n");
    }
}

1;
