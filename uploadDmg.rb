require 'dropbox_api'
require 'yaml'
require 'logger'
require 'find'
require 'pathname'
require 'dotenv/load'
require 'dropbox-deployment'

dropbox_path = '/Builds'

if ENV['DROPBOX_OAUTH_BEARER'].nil?
  puts "\nYou must have an environment variable of `DROPBOX_OAUTH_BEARER` in order to deploy to Dropbox\n\n"
  exit(1)
end

def upload_file(dropbox_client, file, dropbox_path)
  file_name = File.basename(file)
  content = IO.read(file)
  dropbox_client.upload dropbox_path + '/' + file_name, content, mode: :overwrite
end

def upload_directory(dropbox_client, directory_path, dropbox_path)
  Find.find(directory_path) do |file|
    unless File.directory?(file)
      current_file_dir = File.dirname(file)
      if current_file_dir == directory_path
        modified_path = dropbox_path
      else
        # adjust for if we are a subdirectory within the desired saved build folder
        modified_path = dropbox_path + '/' + Pathname.new(current_file_dir).relative_path_from(Pathname.new(directory_path)).to_s
      end
      upload_file(dropbox_client, file, modified_path)
    end
  end
end

dropbox_client = DropboxApi::Client.new
Dir.glob('*.dmg').each do |f|
  # Upload all files
  is_directory = File.directory?(f)
  if is_directory
	  upload_directory(dropbox_client, f, dropbox_path)
  else
    artifact_file = File.open(f)
    upload_file(dropbox_client, artifact_file, dropbox_path)
  end
end
