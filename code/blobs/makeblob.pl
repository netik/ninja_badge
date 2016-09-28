#!/usr/bin/env perl -w
use strict;

print "#include <stdio.h>\n";
print "#include <stdlib.h>\n";
print "#include <string.h>\n";
my @names;
foreach my $f (@ARGV) {
    open FILE, "<$f" or die "$!";
    my @x = $f =~ /((\S+)\/)?(\S+)\.h/;
    if($#x < 2) {
        die "invalid name: $f";
    }
    push @names, $x[2];
    $/ = undef;
    my $x = <FILE>;
    print "$x\n";
}

print "int main(int argc, char *argv[]) {\n";
print "    unsigned int sz = 0\n";
foreach my $n (@names) {
    print "        + sizeof($n)\n";
}
print "    ;\n\n";
print "    unsigned char buf[sz];\n";
print "    unsigned int i = 0;\n";
foreach my $n (@names) {
    print "    memcpy(&buf[i], $n, sizeof($n));\n";
    print "    printf(\"    { " . uc($n) . ", %u, %lu }, \\n\", i, sizeof($n));\n";
    print "    i += sizeof($n);\n";
}
print '    FILE *fp = fopen("blob.out", "w+");
    if(fp == NULL) {
        perror("fopen");
        exit(1);
    }
    fwrite(buf, sz, 1, fp);
    fclose(fp);
';
foreach my $n (@names) {
    print "    printf(\"    " . uc($n) . ",\\n\");\n";
}
print "}\n";
