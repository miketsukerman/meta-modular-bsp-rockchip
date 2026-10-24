# systemd >= 257 ships /etc/profile.d/80-systemd-osc-context.sh, which makes
# bash emit "OSC 3008" hierarchical-context escape sequences
# (\e]3008;start=...;machineid=...;bootid=...\e\\) around every prompt and
# command. Serial terminal programs such as minicom/picocom do not interpret
# OSC sequences and print the payload as literal text, garbling the login
# prompt on the ttyS2 debug console. oe-core enables the 'osc-context'
# PACKAGECONFIG by default; drop it so the profile.d script and its
# tmpfiles.d activation snippet are removed from the image.
PACKAGECONFIG:remove:rsb4810 = "osc-context"
PACKAGECONFIG:remove:aom6881 = "osc-context"
