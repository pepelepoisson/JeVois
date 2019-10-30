module plate(diameter, screwDiameter, thickness) {
    $fn=100;
    t=thickness;
    tt=thickness*2;
    radius=diameter/2;
    centerCylinder=radius*0.8;
    
    union() {
        difference() {
            hull() {
                cylinder(h=t, r=centerCylinder);
                
                translate([radius, radius, 0])
                cylinder(h=t, r=screwDiameter/2+thickness);
            }
            
            translate([radius, radius, 0])
            cylinder(h=t, r=screwDiameter/2);
        }

        difference() {    
            hull() {
                cylinder(h=t, r=centerCylinder);
                
                translate([radius, -radius, 0])
                cylinder(h=t, r=screwDiameter/2+thickness);
            }
            
            translate([radius, -radius, 0])
            cylinder(h=t, r=screwDiameter/2);
        }

        difference() {        
            hull() {
                cylinder(h=t, r=centerCylinder);
                
                translate([-radius, radius, 0])
                cylinder(h=t, r=screwDiameter/2+thickness);
            }
            
            translate([-radius, radius, 0])
            cylinder(h=t, r=screwDiameter/2);
        }

        difference() {            
            hull() {
                cylinder(h=t, r=centerCylinder);
                
                translate([-radius, -radius, 0])
                cylinder(h=t, r=screwDiameter/2+thickness);
            }
            
            translate([-radius, -radius, 0])
            cylinder(h=t, r=screwDiameter/2);
        }
    }
}


module ballSocket (diameter, centerHole, thickness, screwDiameter) {
    $fn=100;
    t=thickness;
    tt=thickness*2;

    difference () {
        union () {
            cylinder(h=diameter/2-t, r=(diameter+t)/2);
            plate(diameter, screwDiameter, thickness);
        }
        cylinder(h=diameter/2, r=centerHole/2);
        translate([0,0,diameter/2+tt])
        sphere(r=diameter/2);
    }
}

module ball (diameter, centerHole, thickness, screwDiameter) {
    $fn=100;
    t=thickness;
    tt=thickness*2;

    translate([0,0,t])
    difference() {
        union () {
            translate([0,0,t])
            sphere(r=diameter/2);
            translate([0,0,-t])
            cylinder(h=tt, r=diameter/2);
            translate([0,0,-t])
            plate(diameter, screwDiameter, thickness);
        }
        translate([0,0,t])
        sphere(r=diameter/2-t);

        cylinder(h=diameter/2+t, r=diameter/2*0.5);

        translate([0,0,-diameter/2])
        cylinder(h=diameter/2+t, r=diameter/2-t);
        translate([-diameter/2,-diameter/2,-t-diameter/2])
        cube([diameter, diameter, diameter/2]);

    }
    
}

module clamp(diameter, centerHole, thickness) {
    $fn=100;
    t=thickness;
    tt=thickness*2;

    translate([0,0,-t])
    difference () {
        translate([0,0,t-diameter/2/3])
        sphere(r=diameter/2-t);
        translate([-diameter/2,-diameter/2,t-diameter])
        cube([diameter, diameter, diameter]);
        cylinder(h=diameter/2+1, r=centerHole/2);
        cylinder(h=diameter/2+1, r=centerHole/2);
    }
}

ballSocket(22, 2.8, 2, 2.8);

translate([35,0,0]) ball(22, 3.8, 2, 2.8);

translate([-30,0,0]) clamp(22, 3.8, 2);

