## @file
# Implementation of Subclass2.
#
# @copy 2002, Bart Schuller &lt;cpan@smop.org&gt;
# $Id: Subclass2.pm,v 1.2 2009/01/08 09:31:44 aeby Exp $

## @class
# A demonstration subclass.
package Subclass2;

use warnings;
use strict;
use base qw(DocumentationDemo);

## @var	    String attribute	some documented variable
# this variable stores some string with unknown use
our $attribute = 'whatever';

## @method void print()
# Print the object as indented ASCII. The printing isn't actually implemented.
# @todo Implement printing.
sub print {
    my($self) = @_;
}

## @method void print_plugin()
# Do some subclass-specific stuff when printing. This time we actually
# implement it.
sub print_plugin {
    my($self) = @_;
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
