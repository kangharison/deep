# HDCP:

> 출처(원문): https://docs.kernel.org/driver-api/mei/hdcp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# HDCP:

ME FW as a security engine provides the capability for setting up
HDCP2.2 protocol negotiation between the Intel graphics device and
an HDC2.2 sink.

ME FW prepares HDCP2.2 negotiation parameters, signs and encrypts them
according the HDCP 2.2 spec. The Intel graphics sends the created blob
to the HDCP2.2 sink.

Similarly, the HDCP2.2 sink’s response is transferred to ME FW
for decryption and verification.

Once all the steps of HDCP2.2 negotiation are completed,
upon request ME FW will configure the port as authenticated and supply
the HDCP encryption keys to Intel graphics hardware.

## mei\_hdcp driver

> The mei\_hdcp driver acts as a translation layer between HDCP 2.2
> protocol implementer (I915) and ME FW by translating HDCP2.2
> negotiation messages to ME FW command payloads and vice versa.

## mei\_hdcp api

int mei\_hdcp\_initiate\_session(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data, struct hdcp2\_ake\_init \*ake\_data)
:   Initiate a Wired HDCP2.2 Tx Session in ME FW

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

`struct hdcp2_ake_init *ake_data`
:   AKE\_Init msg output.

**Return**

0 on Success, <0 on Failure.

int mei\_hdcp\_verify\_receiver\_cert\_prepare\_km(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data, struct hdcp2\_ake\_send\_cert \*rx\_cert, bool \*km\_stored, struct hdcp2\_ake\_no\_stored\_km \*ek\_pub\_km, size\_t \*msg\_sz)
:   Verify the Receiver Certificate AKE\_Send\_Cert and prepare AKE\_Stored\_Km/AKE\_No\_Stored\_Km

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

`struct hdcp2_ake_send_cert *rx_cert`
:   AKE\_Send\_Cert for verification

`bool *km_stored`
:   Pairing status flag output

`struct hdcp2_ake_no_stored_km *ek_pub_km`
:   AKE\_Stored\_Km/AKE\_No\_Stored\_Km output msg

`size_t *msg_sz`
:   size of AKE\_XXXXX\_Km output msg

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_verify\_hprime(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data, struct hdcp2\_ake\_send\_hprime \*rx\_hprime)
:   Verify AKE\_Send\_H\_prime at ME FW.

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

`struct hdcp2_ake_send_hprime *rx_hprime`
:   AKE\_Send\_H\_prime msg for ME FW verification

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_store\_pairing\_info(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data, struct hdcp2\_ake\_send\_pairing\_info \*pairing\_info)
:   Store pairing info received at ME FW

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

`struct hdcp2_ake_send_pairing_info *pairing_info`
:   AKE\_Send\_Pairing\_Info msg input to ME FW

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_initiate\_locality\_check(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data, struct hdcp2\_lc\_init \*lc\_init\_data)
:   Prepare LC\_Init

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

`struct hdcp2_lc_init *lc_init_data`
:   LC\_Init msg output

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_verify\_lprime(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data, struct hdcp2\_lc\_send\_lprime \*rx\_lprime)
:   Verify lprime.

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

`struct hdcp2_lc_send_lprime *rx_lprime`
:   LC\_Send\_L\_prime msg for ME FW verification

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_get\_session\_key(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data, struct hdcp2\_ske\_send\_eks \*ske\_data)
:   Prepare SKE\_Send\_Eks.

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

`struct hdcp2_ske_send_eks *ske_data`
:   SKE\_Send\_Eks msg output from ME FW.

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_repeater\_check\_flow\_prepare\_ack(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data, struct hdcp2\_rep\_send\_receiverid\_list \*rep\_topology, struct hdcp2\_rep\_send\_ack \*rep\_send\_ack)
:   Validate the Downstream topology and prepare rep\_ack.

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

`struct hdcp2_rep_send_receiverid_list *rep_topology`
:   Receiver ID List to be validated

`struct hdcp2_rep_send_ack *rep_send_ack`
:   repeater ack from ME FW.

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_verify\_mprime(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data, struct hdcp2\_rep\_stream\_ready \*stream\_ready)
:   Verify mprime.

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

`struct hdcp2_rep_stream_ready *stream_ready`
:   RepeaterAuth\_Stream\_Ready msg for ME FW verification.

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_enable\_authentication(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data)
:   Mark a port as authenticated through ME FW

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_close\_session(struct [device](../infrastructure.html#c.device "device") \*dev, struct hdcp\_port\_data \*data)
:   Close the Wired HDCP Tx session of ME FW per port. This also disables the authenticated state of the port.

**Parameters**

`struct device *dev`
:   device corresponding to the mei\_cl\_device

`struct hdcp_port_data *data`
:   Intel HW specific hdcp data

**Return**

0 on Success, <0 on Failure

int mei\_hdcp\_component\_match(struct [device](../infrastructure.html#c.device "device") \*dev, int subcomponent, void \*data)
:   compare function for matching mei hdcp.

**Parameters**

`struct device *dev`
:   master device

`int subcomponent`
:   subcomponent to match (I915\_COMPONENT\_HDCP)

`void *data`
:   compare data (mei hdcp device)

**Description**

> The function checks if the driver is i915, the subcomponent is HDCP
> and the grand parent of hdcp and the parent of i915 are the same
> PCH device.

**Return**

* 1 - if components match
* 0 - otherwise
