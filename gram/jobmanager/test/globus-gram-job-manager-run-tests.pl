#! /usr/bin/env perl

use strict;
use Test::Harness;
require 5.005;
use vars qw(@tests);
$|=1;

my $globus_location = $ENV{GLOBUS_LOCATION};
my $contact;

@tests = qw(
    globus-gram-job-manager-submit-test.pl
    globus-gram-job-manager-failure-test.pl
);
if(0 != system("grid-proxy-info -exists -hours 2") / 255)
{
    print STDERR "Security proxy required to run the tests.\n";
    exit 1;
}
chdir "$globus_location/test";

system("globus-personal-gatekeeper -killall >/dev/null 2>/dev/null");
system("globus-personal-gatekeeper -start -log never >/dev/null 2>/dev/null");
chomp($contact = `globus-personal-gatekeeper -list`);
if($? != 0)
{
    print "Could not start gatekeeper\n";
    exit 1;
}

push(@INC, $ENV{GLOBUS_LOCATION} . "/lib/perl");
$ENV{CONTACT_STRING} = $contact;
runtests(@tests);

sub END {
system("globus-personal-gatekeeper -kill \"$contact\" >/dev/null 2>/dev/null");
}
