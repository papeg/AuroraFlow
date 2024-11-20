set hostname [lindex $argv 0]
set ltx_file [lindex $argv 1]

create_project -force tmp

open_hw_manager

connect_hw_server -url $hostname:3121
open_hw_target -xvc_url $hostname:10200

set hw_device [lindex [get_hw_devices] 0]

set_property PROBES.FILE $ltx_file $hw_device
set_property FULL_PROBES.FILE $ltx_file $hw_device

refresh_hw_device -disable_done_check $hw_device

foreach ila [get_hw_ilas] {
    puts $ila
}

set hw_ila [get_hw_ilas hw_ila_3]

puts $hw_ila

set probes [get_hw_probes -of_objects $hw_ila]

set_property CONTROL.TRIGGER_CONDITION AND $hw_ila
set_property TRIGGER_COMPARE_VALUE eq1'b1 [get_hw_probes level0_i/ulp/system_ila_0/inst/net_slot_0_axis_tvalid -of_objects $hw_ila]
set_property TRIGGER_COMPARE_VALUE eq1'b1 [get_hw_probes level0_i/ulp/system_ila_0/inst/net_slot_0_axis_tready -of_objects $hw_ila]

run_hw_ila $hw_ila

list_property $hw_ila

while {[get_property TRIG_STATE $hw_ila] != "TRIGGERED"} {
        after 100  ;# Wait for 100 milliseconds before checking again
}

write_hw_ila_data -hw_ila $hw_ila -file waves.ltx

close_hw_target
disconnect_hw_server

puts "done"
