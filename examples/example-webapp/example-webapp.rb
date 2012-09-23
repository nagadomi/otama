# -*- coding: utf-8 -*-
require 'rubygems'
require 'filemagic'
require 'sinatra'
require 'otama'
require 'yaml'
require 'erb'
require 'gdbm'
require 'cgi'
require 'open-uri'
require 'RMagick'
require 'resolv-replace'
require 'timeout'
require 'fileutils'

ROOT = File.expand_path(File.dirname(__FILE__))
Dir.chdir(ROOT);

class ExampleWebApp
  CONFIG = File.join(ROOT, 'config.yaml')
  DBM_FILE = File.join(ROOT, 'example.gdbm')
  TEAMPLE = File.join(ROOT, 'template.html.erb')
  UPLOAD_DIR = File.join(ROOT, 'upload')
  THUMB_DIR = File.join(ROOT, 'public/thumb')
  RESULT_MAX = 20
  MAX_CONTENT_LENGTH = 10 * 1024 * 1024
  CONTENT_TIMEOUT = 10
  
  Otama.log_level = Otama::LOG_LEVEL_DEBUG

  @@instance = nil
  
  def self.get
    @@instance ||= ExampleWebApp.new
  end
  
  def close
    @db.close
    @otama.close
  end
  def search_by_id(image_id)
    file = image_filename(image_id)
    begin
      send_template(:query_id => image_id,
                    :results => @otama.search(RESULT_MAX, :file => file))
    rescue => e
      send_template(:error_message => e.message)
     end
  end
  def search_by_url(url)
    if (url =~ /^s?https?:\/\//)
      begin
        image = get_remote_content(url)
        send_template(:query_url => url,
                      :results => @otama.search(RESULT_MAX, :data => image)
                      )
      rescue => e
        send_template(:error_message => e.message)
      end
    else
      send_template
    end
  end
  def search_by_file(file)
    begin
      id, path = save_image(file.read)
      send_template(:query_url => "/upload/" + id,
                    :results => @otama.search(RESULT_MAX, :file => path)
                    )
    rescue => e
      send_template(:error_message => e.message)
    end
  end
  def create_by_file(file)
    begin
      blob = file.read
      id, path = save_image(blob)
      id = @otama.insert(:data => blob)
      @db[id] = path
      @otama.pull
      
      send_template(:error_message => "#{id} created.")
    rescue => e
      send_template(:error_message => e.message)
    end
  end
  def create_by_url(url)
    begin
      blob = get_remote_content(url)
      id, path = save_image(blob)
      id = @otama.insert(:data => blob)
      @db[id] = path
      @otama.pull
      send_template(:error_message => "#{id} created.") 
    rescue => e
      send_template(:error_message => e.message)
    end
  end
  def send_template(options = {})
    erb = template
    @query_id = options[:query_id]
    @query_url = options[:query_url]
    @error_message = options[:error_message]
    @results = options[:results] || []
    @random_images = random_images || []
    
    erb.result(binding)
  end
  def image_filename(image_id)
    path = @db[image_id]
    if (File.exist?(path))
      path
    else
      File.join(ROOT, path)
    end
  end
  def upload_filename(file)
    File.join(UPLOAD_DIR, file)
  end
  def thumb_url(filename)
    File.join('/thumb', sprintf("%s.jpg", Otama.id(:file => filename)))
  end
  def thumb_filename(filename)
    File.join(THUMB_DIR, sprintf("%s.jpg", Otama.id(:file => filename)))
  end
  
  private
  def initialize
    unless (File.directory?(UPLOAD_DIR))
      FileUtils.mkdir_p(UPLOAD_DIR)
    end
    unless (File.directory?(THUMB_DIR))
      FileUtils.mkdir_p(THUMB_DIR)
    end
    
    @db = GDBM::open(DBM_FILE, 0600, GDBM::NOLOCK|GDBM::READER|GDBM::WRITER)
    @config = YAML.load_file(CONFIG)
    @otama = Otama.open(@config)
    @otama.pull
  end
  
  def save_image(blob)
    id = Otama.id(:data => blob)
    path = File.join(UPLOAD_DIR, id)
    File.open(path, "wb") do |f|
      f.write blob
    end
    [id, path]
  end
  def template
    @template ||= File.open(TEAMPLE) do |f|
      ERB.new(f.read)
    end
  end
  def random_images(n = RESULT_MAX)
    results = []
    count = @db['COUNT'].to_i
    n.times do |i|
      image_id = @db[rand(count).to_i.to_s]
      if (image_id)
        results << image_id
      end
    end
    results
  end
  def get_remote_content(uri)
    content = nil
    timeout(CONTENT_TIMEOUT) do
      begin
        meta = nil
        http_options = {
          :content_length_proc => Proc.new do |length |
            if (length)
              if (length > MAX_CONTENT_LENGTH)
                raise "content too large"
              end
            else
            end
          end,
        }
        OpenURI.open_uri(uri, http_options) do |f|
          content = f.read
        end
      end
    end
    content
  end
end
trap(:TERM) do
  OtamaInstance.close
  exit
end
trap(:INT) do
  OtamaInstance.close
  exit
end

get '/' do
  controller = ExampleWebApp.get
  controller.send_template
end

get '/search/:image_id' do
  controller = ExampleWebApp.get
  controller.search_by_id(params[:image_id])
end

post '/action' do
  controller = ExampleWebApp.get
  
  if (params[:file] && params[:file][:tempfile])
    if (params[:search])
      controller.search_by_file(params[:file][:tempfile])
    elsif (params[:add])
      controller.create_by_file(params[:file][:tempfile])      
    end
  elsif (params[:url] && !params[:url].empty?)
    if (params[:search])    
      controller.search_by_url(params[:url])
    elsif (params[:add])
      controller.create_by_url(params[:url])
    end
  else
    controller.send_template
  end
end

MIME = FileMagic.new(FileMagic::MAGIC_MIME)
get '/thumb/:image_id.jpg' do
  controller = ExampleWebApp.get
  
  path = controller.image_filename(params[:image_id])
  if (path)
    thumb_path = controller.thumb_filename(path)
    unless (File.exist?(thumb_path))
      img = Magick::Image.read(path).first
      img.resize_to_fit!(120, 120)
      img.format = "JPEG"
      img.write(thumb_path)
    end
    send_file(thumb_path, {
                :type => MIME.file(path),
                :disposition => 'inline'})
  end
end

get '/upload/:hash' do
  controller = ExampleWebApp.get
  
  path = controller.upload_filename(params[:hash])
  if (path)
    send_file(path, {
                :type => MIME.file(path),
                :disposition => 'inline'})
  end
end

