## @file
# Implementation of DocumentationDemo
#
# @copy 2002, Bart Schuller &lt;cpan@smop.org&gt;
# $Id: DocumentationDemo.pm,v 1.1.1.1 2006/01/30 08:51:02 aeby Exp $

## @class
# A demonstration class. Note that the documentation of an entity should
# always start with a short sentence (including a closing full stop).
# @par Making paragraphs
# A paragraph looks like this.
# @par
# A second one, under the same heading
# @note
# notes are also possible
# @par Lists
# Lists can be made quite easily. Possibilities include:
# - Bulleted lists
# - Numbered lists, which:
#   -# look like this
#   -# as you can see
# @par Emphasis
# You cam @em emphasize words very easily. By the way, be sure to always
# escape your \@-signs.
package DocumentationDemo;

use warnings;
use strict;

## @enum Keys
# The allowed properties for our objects.
# Bla.

## @var Keys a
# a is for Adam, who was the first man.

## @var b
# b is for Bernard.

my @Keys = ('a', 'b', 'c');

## @cmethod object new($flavour, %args)
# A normal constructor. Creates a nice fuzzy object.
# @param flavour What taste do you like?
# @param args A hash of key,value pairs to initialize the object with.
# @return A new object.
sub new {
    my($class, $flavour, %args) = @_;
}

## @cmethod object new_from_file($file)
# Another normal constructor from a file. Creates a nice fuzzy object.
# @param file The file to base the object on.
# @return A new object.
sub new {
    my($class, $file) = @_;
}

## @method void print()
# Print the object as indented ASCII. The printing isn't actually implemented.
sub print {
    my($self) = @_;
}

## @method virtual void print_plugin()=0
# Do some class-specific stuff when printing. Must be implemented by
# a subclass.
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
