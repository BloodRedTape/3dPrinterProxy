# Variables
let local_folder = "..\\frontend_flutter\\build\\web"
let zip_file     = "..\\frontend_flutter\\build\\web.tar.gz"
let flutter_project_path = "..\\frontend_flutter"
let remote_user  = "bloodredtape"
let remote_host  = "192.168.1.209"
let remote_path  = "/home/bloodredtape/Pm2/3dPrinterProxy/out/web.tar.gz"
let remote_dest = "/home/bloodredtape/Pm2/3dPrinterProxy/out/"

cd $flutter_project_path

flutter build web --release

let folder_parent = ($local_folder | path dirname)
let folder_name   = ($local_folder | path basename)

tar -C $folder_parent -a -cf $zip_file $folder_name
tar -czf $zip_file -C $local_folder .

scp $zip_file $"($remote_user)@($remote_host):($remote_path)"

ssh $"($remote_user)@($remote_host)" $'tar -xzf ($remote_path) -C ($remote_dest);'