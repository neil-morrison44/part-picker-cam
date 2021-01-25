$upperInnerWidth = 9;
$upperInnerLength = 7.22;

$lowerInnerWidth = 6.92;
$lowerInnerLength = 4.99;

$innerHalfDepth = 4;
$innerFullDepth = 9.33;

$thickness = 1.4;

$piCamWidth = 27;
$piCamDepth = 1.1;
$piCamEdgeWidth = 4;
$piCamEdgeHeight = 6;

$screenScrewRadius = 1.5;

$usbMountWidth = 15.9;
$usbMountDepth = 21.5;
$usbMountHeight = 2;

$lcdScrewDistance = 29;

$cameraCableLength = 60;
$cameraBoxWidth = 9.33;
$cameraBoxDepth = $cameraBoxWidth;
$cameraBoxHeight = 3;
$cameraRadius = 8.23 / 2;

$fs = 0.1;

module upperMount() {
    $offset = ($upperInnerWidth - ($upperInnerLength)) / 2;

    hull() {
        for (i = [ -$offset, $offset ]) {
            translate([ i, 0, 0 ])
                cylinder($innerHalfDepth, ($upperInnerLength * 0.95) / 2,
                         $upperInnerLength / 2);
        }
    }
}

module lowerMount() {
    $offset = ($lowerInnerWidth - ($lowerInnerLength)) / 2;
    hull() {
        for (i = [ -$offset, $offset ]) {
            translate([ 0, 0, $innerHalfDepth ]) {
                translate([ i, 0, 0 ])
                    cylinder($innerFullDepth - $innerHalfDepth,
                             $lowerInnerLength / 2, $lowerInnerLength / 2);
            }
        }
    }
}

// upperMount();
// lowerMount();

module fullMount() {
    rotate([ 0, 180, 0 ]) {
        union() {
            upperMount();
            lowerMount();
        }
    }
}

module otherLeg() {
    translate([ 0, 0, -$innerFullDepth ]) {
        cylinder($innerFullDepth, ($upperInnerLength * 0.95) / 2,
                 $upperInnerLength / 2);
    }
}

module base() {
    translate([ $piCamWidth / 3, -$piCamWidth / 3, 2 ]) {
        cube([ $piCamWidth * 2, $piCamWidth * 2, 4 ], true);
    }
}

module deviceMounts() {
    translate([ -4, -10, 8 ]) rotate([ 30, 0, -45 ]) piCamMount();

    translate([ 30, -20, 21 ]) rotate([ 5, 0, -5 ]) screenMount();

    translate([ -19.5, -28, 4 ]) rotate([ 0, -90, 0 ]) usbMountBox();
}

module piCamMount() {
    module baseCube() {
        translate([ 0, 0, -2 ]) cube(
            [ $piCamWidth + 4, $piCamDepth + 4, $piCamEdgeHeight + 4 ], true);
    }

    difference() {
        baseCube();
        cube([ $piCamWidth, $piCamDepth, $piCamEdgeHeight + 1 ], true);
        cube(
            [
                $piCamWidth - ($piCamEdgeWidth * 2), $piCamDepth + 5,
                $piCamEdgeHeight + 1
            ],
            true);
    }

    module placedCamera(renderLeftHandle = false, renderRightHandle = false) {
        rotate([ 0, 0, 45 ]) translate([ 0, $cameraCableLength * 0.8, 0 ])
            rotate([ 180 - 20, -15, 0 ])
                cameraMount(renderLeftHandle, renderRightHandle);
    }

    placedCamera();

    hull() {
        placedCamera(renderLeftHandle = true);
        translate([ $piCamWidth / 2, $piCamDepth, -2 ])
            cube([ 3, 0.1, ($piCamEdgeHeight + 4) * 0.8 ], true);
    }

    hull() {
        placedCamera(renderRightHandle = true);
        translate([ -$piCamWidth / 2, $piCamDepth, -2 ])
            cube([ 3, 0.1, ($piCamEdgeHeight + 4) * 0.8 ], true);
    }
}

module screenMount() {
    rotate([ 90, 0, 0 ]) difference() {
        cube([ 5 + ($screenScrewRadius), 38, 4 ], true);
        translate([ 0, 0, -3 ]) {
            for (i = [ -$lcdScrewDistance / 2, $lcdScrewDistance / 2 ]) {
                translate([ 0, i, 0 ]) {
                    cylinder(5 + 1, $screenScrewRadius, $screenScrewRadius);
                    translate([ 0, 0, 2 ]) cylinder(4, $screenScrewRadius * 1.5,
                                                    $screenScrewRadius * 2);
                }
            }
        }
    }
}

module usbMountBox() {
    difference() {
        translate([ -($usbMountDepth + 1) * 0.25, 0, 0 ]) cube(
            [
                ($usbMountDepth + 1) * 0.5, $usbMountWidth + 3,
                $usbMountHeight + 3
            ],
            true);
        union() {
            cube([ $usbMountDepth, $usbMountWidth, $usbMountHeight ], true);
            minkowski() {
                translate([ 0, 0, 1 ]) cube(
                    [
                        $usbMountDepth + 1.1, $usbMountWidth / 1.25,
                        $usbMountHeight + 2
                    ],
                    true);
                sphere(r = 0.4);
            }
        }
    }
}

module legs() {
    fullMount();
    translate([ 20, -5, 0 ]) otherLeg();
    translate([ 5, -20, 0 ]) otherLeg();
}

module betterBase() {
    hull() {
        intersection() {
            base();
            deviceMounts();
        }
        intersection() {
            base();
            legs();
        }
    }
}

module camera() {
    cube([ $cameraBoxWidth, $cameraBoxDepth, $cameraBoxHeight ], true);
    cylinder($cameraBoxHeight * 2, $cameraRadius, $cameraRadius);
}

module cameraMount(renderRightHandle = false, renderLeftHandle = false) {
    $handleThickness = 1;

    module handle() {
        cube([ $handleThickness, 12 * 0.8, $handleThickness ], true);
    }

    if (renderRightHandle || renderLeftHandle) {
        if (renderRightHandle) {
            translate([ 5.9 + ($handleThickness / 2), 0, 0 ])

                handle();
        }
        if (renderLeftHandle) {
            translate([ -(5.9 + ($handleThickness / 2)), 0, 0 ]) handle();
        }
    } else {
        difference() {
            cube([ 12, 12, 4 ], true);
            hull() {
                for (i = [ 0, 5 ]) {
                    translate([ 0, i, 0 ]) cube(
                        [ $cameraBoxWidth, $cameraBoxDepth, $cameraBoxHeight ],
                        true);
                }
            }
            hull() {
                for (i = [ 0, 5 ]) {
                    translate([ 0, i, 0 ]) cylinder(
                        $cameraBoxHeight * 2, $cameraRadius, $cameraRadius);
                }
            }
        }
    }
}

legs();
betterBase();
deviceMounts();
