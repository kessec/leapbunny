grep "Setting for pressure_curve:" new-calib* > greps
grep "Setting for max_tnt_down:" new-calib* >> greps
grep "g at" new-calib* >> greps
grep "Mode .*g: P=" new-calib* >> greps

