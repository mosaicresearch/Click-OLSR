#!/usr/bin/perl -w

# script to generate OLSR configurations for Click.
# Joachim Klein (2004)
# Modified by Johan Bergs (2005)

use strict;

my $prog = "make-olsr-config.pl";


sub usage() {
	print "usage: $prog -i IF -a ADDR [OPTIONS]
  Generate a Click OLSR configuration to run on interface IF with IP address ADDR.

Options:
   -h, --help                   Print this message and exit.
   
   -k, --kernel                 Run in kernel.  Only works on Linux. (default)
   -u, --userlevel              Run in userlevel.
   -s, --simulator              Run in ns-click simulator.
   
   -n  --number N               Use n interfaces defaults to 1
   -i, --interface IF1..IFn     Use interface IF.  Required option.
   -a, --address A1..An         Use IP address A.  Required option.

   --ether ETH1..ETHn           Use Ethernet address ETH for interface.  If not specified,
                                the address will be dynamically discovered.

  Optional OLSR parameters.
   --HELLO-Interval T(msec)     Hello Interval [default: 1500]
   --HELLO-Jitter T(msec)       Jitter Interval for Hello messages [default 1/10 Hello Interval]
   --TC-Interval T(msec)        TC Interval [default: 5000]
   --TC-Jitter T(msec)          Jitter Interval for TC messages [default 1/10 TC Interval]
   --MID-Interval T(msec)       Mid Interval [default: 5000]
   --MID-Jitter T(msec)         Jitter Interval for MID messages [default 1/10 MID Interval]
   --neighb-hold-time T (msec)  Neighbor Holding time [default: 3*Hello Interval]
   --top-hold-time T (msec)     Topology Holding time [default: 3*TC Interval]
   --mid-hold-time T (msec)     MID Holding time [default: 3*MID Interval]
   --dup-hold-time T (sec)      duplicate Holding time [default: 30s]
   --Jitter T (sec)             max Jitter time for forwarded OLSR control messages [default: hello_Interval/4000]
   --spread-init (boolean)      Initial emission of hello/Mid messages starts after random time
                                between 0 and respective emission Interval [default: false]
   --additional-hello-msgs      Send more than just necessary messages to react faster to link failures
   --additional-tc-msgs         Send more than just necessary messages to react faster to topology changes
   --Jitter-all	T (sec)		old way: all packets are jittered before output on interface x default: 0
   ";
}

my @orig_args = @ARGV;

foreach my $arg (@ARGV) {
	if ($arg eq "-h" || $arg eq "--help") {
		usage();
		exit(0);
	}
}

sub bail($) {
	print STDERR "$prog: ", shift, "\n";
	exit 1;
}

sub get_arg() {
	my $arg = shift @ARGV;

	if (!defined($arg)) {
		print STDERR "$prog: Missing argument parameter\n";
		usage();
		exit 1;
	}

	return $arg;
}

my @ifname = "";
my $in_kernel = 0;
my $in_userlevel = 0;
my $in_simulator = 0;
my $n=1;
my @addr = "";
my @eth = "";
my $tc_interval = 5000;
my $tc_jitter=-1;
my $mid_interval = 5000;
my $mid_jitter=-1;
my $hello_interval = 1500;
my $hello_jitter=-1;
my $Jitter=-1;
my $Jitter_all=-1;
my $additional_hello_msgs = "false";
my $additional_tc_msgs = "false";
my $neighb_hold_time=0;
my $top_hold_time=0;
my $mid_hold_time=0;
my $dup_hold_time=0;
my $spread_init="false";
my $hna=1;
my $hna_gen=0;

while (scalar(@ARGV) > 0) {
	my $arg = shift @ARGV;

	if ($arg eq "--help" || $arg eq "-h") {
		usage();
		exit 0;
	}
	elsif ($arg eq "--number" || $arg eq "-n") {
		$n = get_arg();
	}
	elsif ($arg eq "--interface" || $arg eq "-i") {
		for(my $i = 0; $i < $n; $i++) {
			$ifname[$i] = get_arg();
		}
	}
	elsif ($arg eq "--address" || $arg eq "-a") {
		for(my $i = 0; $i < $n; $i++) {
			$addr[$i] = get_arg();
		}
	}
	elsif ($arg eq "--ether") {
		for(my $i = 0; $i < $n; $i++) {
			$eth[$i] = get_arg();
		}
	}
	elsif ($arg eq "--additional-hello-msgs") {
		$additional_hello_msgs = "true";
	}
	elsif ($arg eq "--additional-tc-msgs") {
		$additional_tc_msgs = "true";
	}
	elsif ($arg eq "--spread-init") {
		$spread_init = "true";
	}
	elsif ($arg eq "--hna") {
		$hna = 1;
	}
	elsif ($arg eq "--hna-gen") {
		$hna_gen = 1;
	}
	elsif ($arg eq "--kernel" || $arg eq "-k") {
		$in_kernel = 1;
	}
	elsif ($arg eq "--userlevel" || $arg eq "-u") {
		$in_userlevel = 1;
	}
	elsif ($arg eq "--simulator" || $arg eq "-s") {
		$in_simulator = 1;
	}
	elsif ($arg eq "--HELLO-Interval") {
		$hello_interval = get_arg();
	}
	elsif ($arg eq "--TC-Interval") {
		$tc_interval = get_arg();
	}
	elsif ($arg eq "--MID-Interval") {
		$mid_interval = get_arg();
	}
	elsif ($arg eq "--HELLO-Jitter") {
		$hello_jitter = get_arg();
	}
	elsif ($arg eq "--TC-Jitter") {
		$tc_jitter = get_arg();
	}
	elsif ($arg eq "--MID-Jitter") {
		$mid_jitter = get_arg();
	}
	elsif ($arg eq "--Jitter-all") {
		$Jitter_all = get_arg();
	}
	elsif ($arg eq "--neighb-hold-time") {
		$neighb_hold_time = get_arg();
	}
	elsif ($arg eq "--top-hold-time") {
		$top_hold_time = get_arg();
	}
	elsif ($arg eq "--mid-hold-time") {
		$mid_hold_time = get_arg();
	}
	elsif ($arg eq "--dup-hold-time") {
		$dup_hold_time = get_arg();
	}
	elsif ($arg eq "--Jitter") {
		$Jitter = get_arg();
	}
	else {
		print STDERR "$prog: Unknown argument `$arg'\n";
		usage();
		exit 1;
	}
}

if ($neighb_hold_time eq 0) {
	$neighb_hold_time = 3 * $hello_interval;
}

if ($top_hold_time eq 0) {
	$top_hold_time = 3 * $tc_interval;
}

if ($mid_hold_time eq 0) {
	$mid_hold_time = 3 * $mid_interval;
}

if ($dup_hold_time eq 0) {
	$dup_hold_time = 30;
}

if ($tc_jitter < 0) {
	$tc_jitter = $tc_interval/10;
}

if ($hello_jitter < 0) {
	$hello_jitter = $hello_interval/10;
}

if ($mid_jitter < 0) {
	$mid_jitter = $mid_interval/10;
}

for(@ifname) {
	if ($_ eq "") {
		bail("No interface specified, try --help for more info.");
	}
 }

 if ($in_simulator != 1) {
 	for(@addr) {
 		if ($_ eq "") {
 			bail("No IP address specified, try --help for more info");
 		}

 		if ($_ !~ /\d+\.\d+\.\d+\.\d+/) {
 			bail("IP address `$_' has a bad format (should be like `a.b.c.d')");
 		}
 	}

 	for(@eth) {
 		if ($_ ne "" && $_ !~ /([a-f0-9][a-f0-9]?:){5}[a-f0-9][a-f0-9]?/i) {
 			bail("Ethernet address `$_' has a bad format");
 		}
 	}
 }

 
if ($in_kernel + $in_userlevel  +$in_simulator > 1) {
	bail ("can only run in either kernel mode, userlevel or simulator");
}

if (!$in_kernel) {
	my $use_feedback = 0;
}

sub check_param($$$) {
	my $n = shift;
	my $v = shift;
	my $min = shift;

	if ($v =~ /(\-?\d+)/) {
		$v = $1;

		if ($v < $min) {
			bail("Argument to --$n must be >= $min");
		}
	}
	else {
		bail("Argument to --$n must be numeric");
	}
}

my $suffix="";

print "elementclass OLSRnode {
\t";

for(my $i = 0; $i < $n; $i++) {
	print "\$my_ip$i, \$my_ether$i, ";
}

print "\$hello_period, \$tc_period, \$mid_period, \$jitter, \$n_hold, \$t_hold, \$m_hold, \$d_hold | 
";

if ($in_simulator eq 1) {
	print "
	tolocal::ToSimDevice(tap0);
	fromlocal::FromSimDevice(tap0, 4096)
		-> fromhost_cl::Classifier(12/0806, 12/0800);

	fromhost_cl[0]
		-> ARPResponder(0.0.0.0/0 0:1:2:3:4:5)
		-> tolocal;
		
	";
	$suffix="simnet";
}
elsif ($in_userlevel eq 1) {
	print "
	fromlocal::FromHost(fake0, \$my_ip0/24)
		-> fromhost_cl::Classifier(12/0806, 12/0800);

	fromhost_cl[0]
		-> ARPResponder(0.0.0.0/0 0:1:2:3:4:5)
		-> tolocal::ToHost(fake0);
		
	";

	$suffix="eth";
}
elsif ($in_kernel eq 1) {
	print "
	tolocal::ToHost;
	fromlocal::FromHost (fake0, \$my_ip0/24)
		-> fromhost_cl::Classifier(12/0806, 12/0800);

	fromhost_cl[0]
		-> ARPResponder(0.0.0.0/0 0:1:2:3:4:5) -> tolocal;
	";
	$suffix="eth";
}

my $arpn;

if ($in_kernel eq 1) {
	$arpn = $n + 1;
} else {
	$arpn = $n;
}

print "
	joinInput::Join($n);

	forward::OLSRForward(\$d_hold, duplicate_set, neighbor_info, interface_info, interfaces, \$my_ip0)
	interface_info::OLSRInterfaceInfoBase(routing_table, interfaces);
	interfaces::OLSRLocalIfInfoBase(";

for(my $i = 0; $i < $n - 1; $i++) {
	print "\$my_ip$i, ";
}

print "\$my_ip", $n-1, ")

	// in kernel ARP responses are copied to each ARPQuerier and the host.

	arpt::Tee(",$arpn,");";


for(my $i = 0; $i < $n; $i++) {
	if ($in_simulator eq 1) {
		print "
	in$i\::FromSimDevice(",$ifname[$i],",4096)
	todevice$i\::ToSimDevice(",$ifname[$i],");
	out$i\::Queue(20)
		-> todevice$i;
		";
	}
	elsif (($in_userlevel eq 1) || ($in_kernel eq 1)) {
		print "
	in$i\::FromDevice(",$ifname[$i],")
	todevice$i\::ToDevice(",$ifname[$i],");
	out$i\::Queue(100)
		-> todevice$i;
		";
	}
	
	print "
	// Input and output paths for ",$ifname[$i],"
	c$i\::Classifier(12/0806 20/0001, 12/0806 20/0002, 12/0800, -);
	in$i	-> SetTimestamp
		-> HostEtherFilter(\$my_ether$i, DROP_OWN false, DROP_OTHER true)
		-> c$i;

	joindevice$i\::Join(3)
		-> out$i;
		
	c$i\[0]	-> ar$i\::OLSRARPResponder(\$my_ip$i \$my_ether$i)
		-> [1]joindevice$i;
		
	arpq$i\::OLSRARPQuerier(\$my_ip$i, \$my_ether$i)
		-> [2]joindevice$i;
		
	c$i\[1]	-> arpt;
	arpt[$i]	-> [1]arpq$i;

	c$i\[2]	-> Paint($i)
		-> [$i]joinInput;

	c$i\[3]	-> Discard;

	output$i\::Join(2)
		-> OLSRAddPacketSeq(\$my_ip$i)
		-> UDPIPEncap(\$my_ip$i , 698, 255.255.255.255, 698)
		-> EtherEncap(0x0800, \$my_ether$i , ff:ff:ff:ff:ff:ff)
		-> ";

 	if ($Jitter_all > 0) {
 		print "Queue(JUall$i)
 		-> JUall$i\::JitterUnqueue($Jitter_all)
 		-> ";
 	}

	print "[0]joindevice$i;

	hello_generator$i\::OLSRHelloGenerator(\$hello_period, \$n_hold, link_info, neighbor_info, interface_info, forward, \$my_ip$i, \$my_ip0)
		-> [1]output$i

	";
}

if ($in_kernel eq 1) {
	print "
	arpt[",$n,"]	-> tolocal;
";
}

print "
	duplicate_set::OLSRDuplicateSet
	olsrclassifier::OLSRClassifier(duplicate_set, interfaces, \$my_ip0)
	check_header::OLSRCheckPacketHeader(duplicate_set)

	// Input
	
	ip_classifier::IPClassifier(udp port 698,-)
	get_src_addr::GetIPAddress(12)
	get_dst_addr::GetIPAddress(16)

	joinInput
		-> MarkEtherHeader
		-> Strip(14)
		-> CheckIPHeader
		-> ip_classifier

	ip_classifier[0]
		-> get_src_addr
		-> Strip(28)
		-> check_header
		-> olsrclassifier

	check_header[1]
		-> Discard

	join_fw::Join(4)

	// olsr control message handling
	
	neighbor_info::OLSRNeighborInfoBase(routing_table, tc_generator, hello_generator0, link_info, interface_info, \$my_ip0, ADDITIONAL_HELLO $additional_hello_msgs);
	topology_info::OLSRTopologyInfoBase(routing_table);
	link_info::OLSRLinkInfoBase(neighbor_info, interface_info, duplicate_set, routing_table,tc_generator)
";

if ($hna < 1) {
	print "
	routing_table::OLSRRoutingTable(neighbor_info, link_info, topology_info, interface_info,\$my_ip0);
	olsrclassifier[4]
		-> [2]join_fw
	";
}
else {
	print "
	association_info::OLSRAssociationInfoBase(routing_table);
	routing_table::OLSRRoutingTable(neighbor_info, link_info, topology_info, interface_info, interfaces, association_info, linear_ip_lookup, \$my_ip0);
	process_hna::OLSRProcessHNA(association_info, neighbor_info, routing_table, \$my_ip0);
	olsrclassifier[4]
		-> process_hna

	process_hna[1]
		-> Discard

	process_hna[0]
		-> [2]join_fw
	";
}

print "
	process_hello::OLSRProcessHello(\$n_hold, link_info, neighbor_info, interface_info, routing_table, tc_generator, interfaces, \$my_ip0);
	process_tc::OLSRProcessTC(topology_info, neighbor_info, interface_info, routing_table, \$my_ip0);
	process_mid::OLSRProcessMID(interface_info, routing_table);

	olsrclassifier[0]
		-> Discard
	olsrclassifier[1]
		-> AddARPEntry(arpq0)
		-> process_hello
		-> Discard

	olsrclassifier[2]
		-> process_tc

	olsrclassifier[3]
		-> process_mid

	olsrclassifier[5]
		-> [3]join_fw

	process_tc[0]
		-> [0]join_fw

	process_tc[1]
		-> Discard

	process_mid[0]
		-> [1]join_fw

	process_mid[1]
		-> Discard
";


if ($Jitter > 0) {
	print "
	forward[0]
		-> [0]joinjitter::Join(2)
		-> broadcast::Tee;

	forward[2]
		-> Queue (JUfw,200)
		-> JUfw::JitterUnqueue($Jitter)
		-> [1]joinjitter
	";
}

print "

	join_fw	-> [0]forward;
	broadcast::Tee
	forward[0]
		-> broadcast
";

for (my $i = 0; $i < $n; $i++) {
	print "
	broadcast[$i]
		->[0]output$i\n";
}

print "
	forward[1]
		-> Discard

	//handling of other ip packets

	dst_classifier::IPClassifier(dst \$my_ip0, -);";

if ($hna < 1) {
	print "
	get_next_hop::OLSRGetNextHop(routing_table,interfaces) ";
}
else {
	print "
	linear_ip_lookup::OLSRLinearIPLookup()
	";
}

print "
	join_cl::Join(2);
	ttl::DecIPTTL
	fromhost_cl[1]
		-> Strip(14)
		-> MarkIPHeader
		-> [0]join_cl;

	join_cl	-> dst_classifier
	
	ip_classifier[1]
		-> [1]join_cl

	dst_classifier[0]
		-> EtherEncap(0x0800, 1:1:1:1:1:1, 0:1:2:3:4:5)
		-> tolocal

	dst_classifier[1]
		-> ttl";
		
if ($hna < 1) {
	print "
	ttl[0]	-> get_next_hop
	ttl[1]	-> Discard
	get_next_hop[0]
		-> nexthopswitch //[0]arpquerier
		
	get_next_hop[1]
		-> Discard
	";
}
else {
	print "
	ttl[0]	-> get_dst_addr
		-> linear_ip_lookup

	ttl[1]	-> Discard

	";
}

for(my $i = 0; $i < $n; $i++) {
	print "
	linear_ip_lookup[$i]
		-> [0]arpq$i\n";
}

print "
	mid_generator::OLSRMIDGenerator(\$mid_period, \$m_hold,interfaces)
	tc_generator::OLSRTCGenerator(\$tc_period, \$t_hold, neighbor_info, \$my_ip0, ADDITIONAL_TC $additional_tc_msgs)
";

if ($hna_gen <1 ) {
	print "
	joinforward::Join(2)
	";
}
else {
	print"
	hna_generator::OLSRHNAGenerator(\$tc_period, \$t_hold, \$my_ip0)
	joinforward::Join(3)
	hna_generator
		-> [2]joinforward
	";
}

print "
	tc_generator
		-> [0]joinforward
		-> [1]forward

	mid_generator
		-> [1]joinforward

} //end of compound element

//insert node_address, hello_interval, tc_interval, mid_interval, jitter
";


for(my $i = 0;$i < $n; $i++) {
	print "AddressInfo(my_ether$i ",$ifname[$i],":$suffix);\n";
}
 
if ($in_simulator != 1) {
	for(my $i = 0; $i < $n; $i++) {
		print "AddressInfo(my_ip$i ",$addr[$i],");\n";
	}
}

print "
OLSRnode(";

if ($in_simulator) {
	for(my $i = 0; $i < $n; $i++) {
		print "my_ether$i,my_ether$i,";
	}
}
else {
	for(my $i = 0;$i < $n; $i++) {
		print "my_ip$i,my_ether$i,";
	}
}

print "$hello_interval, $tc_interval, $mid_interval, $Jitter, $neighb_hold_time, $top_hold_time, $mid_hold_time, $dup_hold_time);\n";
