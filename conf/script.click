elementclass OLSRnode {
	$my_ip0, $my_ether0, $hello_period, $tc_period, $mid_period, $jitter, $n_hold, $t_hold, $m_hold, $d_hold | 

	fromlocal::FromHost(fake0, $my_ip0/24)
		-> fromhost_cl::Classifier(12/0806, 12/0800);

	fromhost_cl[0]
		-> ARPResponder(0.0.0.0/0 0:1:2:3:4:5)
		-> tolocal::ToHost(fake0);
		
	
	joinInput::Join(1);

	forward::OLSRForward($d_hold, duplicate_set, neighbor_info, interface_info, interfaces, $my_ip0)
	interface_info::OLSRInterfaceInfoBase(routing_table, interfaces);
	interfaces::OLSRLocalIfInfoBase($my_ip0)

	// in kernel ARP responses are copied to each ARPQuerier and the host.

	arpt::Tee(1);
	in0::FromDevice(en0)
	todevice0::ToDevice(en0);
	out0::Queue(100)
		-> todevice0;
		
	// Input and output paths for en0
	c0::Classifier(12/0806 20/0001, 12/0806 20/0002, 12/0800, -);
	in0	-> SetTimestamp
		-> HostEtherFilter($my_ether0, DROP_OWN false, DROP_OTHER true)
		-> c0;

	joindevice0::Join(3)
		-> out0;
		
	c0[0]	-> ar0::OLSRARPResponder($my_ip0 $my_ether0)
		-> [1]joindevice0;
		
	arpq0::OLSRARPQuerier($my_ip0, $my_ether0)
		-> [2]joindevice0;
		
	c0[1]	-> arpt;
	arpt[0]	-> [1]arpq0;

	c0[2]	-> Paint(0)
		-> [0]joinInput;

	c0[3]	-> Discard;

	output0::Join(2)
		-> OLSRAddPacketSeq($my_ip0)
		-> UDPIPEncap($my_ip0 , 698, 255.255.255.255, 698)
		-> EtherEncap(0x0800, $my_ether0 , ff:ff:ff:ff:ff:ff)
		-> [0]joindevice0;

	hello_generator0::OLSRHelloGenerator($hello_period, $n_hold, link_info, neighbor_info, interface_info, forward, $my_ip0, $my_ip0)
		-> [1]output0

	
	duplicate_set::OLSRDuplicateSet
	olsrclassifier::OLSRClassifier(duplicate_set, interfaces, $my_ip0)
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
	
	neighbor_info::OLSRNeighborInfoBase(routing_table, tc_generator, hello_generator0, link_info, interface_info, $my_ip0, ADDITIONAL_HELLO false);
	topology_info::OLSRTopologyInfoBase(routing_table);
	link_info::OLSRLinkInfoBase(neighbor_info, interface_info, duplicate_set, routing_table,tc_generator)

	association_info::OLSRAssociationInfoBase(routing_table);
	routing_table::OLSRRoutingTable(neighbor_info, link_info, topology_info, interface_info, interfaces, association_info, linear_ip_lookup, $my_ip0);
	process_hna::OLSRProcessHNA(association_info, neighbor_info, routing_table, $my_ip0);
	olsrclassifier[4]
		-> process_hna

	process_hna[1]
		-> Discard

	process_hna[0]
		-> [2]join_fw
	
	process_hello::OLSRProcessHello($n_hold, link_info, neighbor_info, interface_info, routing_table, tc_generator, interfaces, $my_ip0);
	process_tc::OLSRProcessTC(topology_info, neighbor_info, interface_info, routing_table, $my_ip0);
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


	join_fw	-> [0]forward;
	broadcast::Tee
	forward[0]
		-> broadcast

	broadcast[0]
		->[0]output0

	forward[1]
		-> Discard

	//handling of other ip packets

	dst_classifier::IPClassifier(dst $my_ip0, -);
	linear_ip_lookup::OLSRLinearIPLookup()
	
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
		-> ttl
	ttl[0]	-> get_dst_addr
		-> linear_ip_lookup

	ttl[1]	-> Discard

	
	linear_ip_lookup[0]
		-> [0]arpq0

	mid_generator::OLSRMIDGenerator($mid_period, $m_hold,interfaces)
	tc_generator::OLSRTCGenerator($tc_period, $t_hold, neighbor_info, $my_ip0, ADDITIONAL_TC false)

	joinforward::Join(2)
	
	tc_generator
		-> [0]joinforward
		-> [1]forward

	mid_generator
		-> [1]joinforward

} //end of compound element

//insert node_address, hello_interval, tc_interval, mid_interval, jitter
AddressInfo(my_ether0 en0:eth);
AddressInfo(my_ip0 1.2.3.4);

OLSRnode(my_ip0,my_ether0,1500, 5000, 5000, -1, 4500, 15000, 15000, 30);
