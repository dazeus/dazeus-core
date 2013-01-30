## @file
# Implementation of Subclass1.
#
# @copy 2002, Bart Schuller &lt;cpan@smop.org&gt;
# $Id: Subclass1.pm,v 1.1.1.1 2006/01/30 08:51:02 aeby Exp $

## @class
# A demonstration subclass.
package Subclass1;

use warnings;
use strict;
use base qw(DocumentationDemo);
use Helper;

## @method Helper munge(Helper h)
# munge a Helper.
# @param h the Helper to be munged
sub munge {
    my($self, $helper) = @_;
}

## @method void print()
# Print the object as indented ASCII. The printing isn't actually implemented.
sub print {
    my($self) = @_;
}

## @method void print_plugin()
# Do some subclass-specific stuff when printing. This time we actually
# implement it.
sub print_plugin {
    my($self) = @_;
    $self->print_helper(upcase($self));
}

## @method protected void print_helper()
# Print the object as indented ASCII. The printing isn't actually implemented.
sub print_helper {
    my($self) = @_;
}

## @fn private void debug(@args)
# A simple function for debugging. Prints the arguments to STDERR.
# @param args The stuff to be printed.
sub debug {
    my(@args) = @_;
}

1;
