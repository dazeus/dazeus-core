## @file
# Useful helpers.

## @class
# Container for helper functions.
package Helper;

## @cmethod Helper new()
# Create a new Helper.
sub new {
}

## @fn $ upcase($string)
# Upcase a string.
# @param string a string to be upcased
# @return the upper-case version of string.
sub upcase {
    return uc(shift);
}

1;
