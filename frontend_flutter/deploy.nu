# Variables
let local_folder = ".\\build\\web"
let zip_file     = ".\\build\\web.tar.gz"
let remote_user  = "bloodredtape"
let remote_host  = "192.168.1.209"
let remote_path  = "/home/bloodredtape/Pm2/3dPrinterProxy/out/web.tar.gz"
let remote_dest = "/home/bloodredtape/Pm2/3dPrinterProxy/out/"

flutter build web --release

let folder_parent = ($local_folder | path dirname)
let folder_name   = ($local_folder | path basename)

tar -C $folder_parent -a -cf $zip_file $folder_name
tar -czf $zip_file -C $local_folder .

scp $zip_file $"($remote_user)@($remote_host):($remote_path)"

ssh $"($remote_user)@($remote_host)" $'tar -xzf ($remote_path) -C ($remote_dest); pm2 restart 3d_proxy'