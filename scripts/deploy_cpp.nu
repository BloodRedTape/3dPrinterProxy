# Variables
let remote_user  = "bloodredtape"
let remote_host  = "192.168.1.209"
let repo_path = "/home/bloodredtape/Pm2/3dPrinterProxy/"

ssh $"($remote_user)@($remote_host)" $'cd ($repo_path); git pull; git submodule update --init --recursive; cd build; ./build.sh && pm2 restart 3d_proxy'