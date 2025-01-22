# Queue Traffic Simulator

### Problem Description
A ***traffic junction*** connects two major roads, forming a central point where vehicles must choose
one of three alternative paths to continue (visual representation is shown in figure 1). The traffic
management system must handle the following scenarios:
- #### Normal Condition:
    Vehicles at the junction are served equally from each lane. The system should ensure that vehicles are dispatched fairly.
- #### High-Priority Condition:
    If one of the roads (referred to as the priority road) accumulatesmore than 10 waiting vehicles, that road should be served first until the count drops below 5. Afterward, normal conditions resume.

#
