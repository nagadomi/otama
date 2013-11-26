# -*- coding: utf-8 -*-
require 'rubygems'
require 'filemagic'
require 'sinatra/base'
require 'otama'
require 'yaml'
require 'erb'
require 'cgi'
require 'open-uri'
require 'RMagick'
require 'resolv-replace'
require 'timeout'
require 'fileutils'

unless defined?(Otama::KVS)
  warn "otama does not have KVS API."
  warn "please re-configure with --enable-leveldb option."
  exit(1)
end

class ExampleWebApp < Sinatra::Base
  Otama.log_level = Otama::LOG_LEVEL_DEBUG
  ROOT = File.expand_path(File.dirname(__FILE__))
  APP_CONFIG = './example-webapp.yaml'
  OTAMA_CONFIG = File.join(ROOT, 'config.yaml')
  DBM_FILE = File.join(ROOT, 'data/example.ldb')
  THUMB_SIZE = 96
  
  ENABLE_UPLOAD_SEARCH = true
  ENABLE_INSERT = false
  COLUMNS = 6
  ROWS = 5
  UPLOAD_DIR = File.join(ROOT, 'upload')
  THUMB_DIR = File.join(ROOT, 'public/thumb')
  MAX_UPLOAD_SIZE = 10 * 1024 * 1024
  URL_TIMEOUT = 10
  
  def self.load_config
    config = YAML.load_file(APP_CONFIG)
    set :enable_upload_search, config['enable_upload_search'] || ENABLE_UPLOAD_SEARCH
    set :enable_insert, config['enable_insert'] || ENABLE_INSERT
    set :columns, config['columns'] || COLUMNS
    set :rows, config['rows'] || ROWS
    set :thumb_size, THUMB_SIZE
    set :upload_dir, config['upload_dir'] || UPLOAD_DIR
    set :thumb_dir, config['thumb_dir'] || THUMB_DIR
    set :max_upload_size, config['max_upload_size'] || MAX_UPLOAD_SIZE
    set :url_timeout, config['url_timeout'] || URL_TIMEOUT
    set :result_max, settings.columns * settings.rows
  end
  def color_weight_support?
    ["sim", "bovw2k_sboc", "bovw8k_sboc",
     "bovw2k_boc", "bovw8k_boc",
     "lmca_vlad_hsv", "lmca_vlad_colorcode"
    ].include?(settings.config["driver"]["name"])
  end
  configure do
    Dir.chdir(ROOT)
    load_config
    
    set :db, Otama::KVS::open(DBM_FILE)
    set :config, YAML.load_file(OTAMA_CONFIG)
    set :otama, Otama.open(settings.config)
    set :mime, FileMagic.new(FileMagic::MAGIC_MIME)
    settings.otama.pull
    
    trap(:TERM) do
      settings.otama.close
      settings.db.close      
      exit
    end
    trap(:INT) do
      settings.otama.close
      settings.db.close
      exit
    end
    unless (File.directory?(settings.upload_dir))
      FileUtils.mkdir_p(settings.upload_dir)
    end
    unless (File.directory?(settings.thumb_dir))
      FileUtils.mkdir_p(settings.thumb_dir)
    end
  end
  get '/' do
    send_template
  end
  get '/search/:image_id' do
    search_by_id(params[:image_id])
  end
  post '/action' do
    if (params[:file] && params[:file][:tempfile])
      if (params[:search])
        search_by_file(params[:file][:tempfile])
      elsif (params[:add])
        create_by_file(params[:file][:tempfile])      
      end
    elsif (params[:url] && !params[:url].empty?)
      if (params[:search])    
        search_by_url(params[:url])
      elsif (params[:add])
        create_by_url(params[:url])
      end
    else
      send_template
    end
  end
  get '/thumb/:image_id.jpg' do
    path = image_filename(params[:image_id])
    if (path)
      thumb_path = thumb_filename(params[:image_id])
      unless (File.exist?(thumb_path))
        img = Magick::Image.read(path).first
        img.resize_to_fit!(settings.thumb_size, settings.thumb_size)
        img.format = "JPEG"
        img.write(thumb_path)
      end
      send_file(thumb_path, {
                  :type => settings.mime.file(path),
                  :disposition => 'inline'})
    end
  end
  get '/upload/:hash' do
    path = upload_filename(params[:hash])
    if (path)
      send_file(path, {
                  :type => settings.mime.file(path),
                  :disposition => 'inline'})
    end
  end
  
  private
  def search_by_id(image_id)
    file = image_filename(image_id)
    begin
      query = {:id => image_id}
      if (color_weight = get_color_weight)
        query[:color_weight] = color_weight
      end
      send_template(:query_id => image_id,
                    :results => settings.otama.search(settings.result_max, query))
    rescue => e
      puts e.backtrace
      send_template(:error_message => e.message)
     end
  end
  def search_by_url(url)
    raise "disabled" unless (settings.enable_upload_search)
    if (url =~ /^s?https?:\/\//)
      begin
        image = get_remote_content(url)
        query = {:data => image}
        if (color_weight = get_color_weight)        
          query[:color_weight] = color_weight
        end
        send_template(:query_url => url,
                      :results => settings.otama.search(settings.result_max, query))
      rescue => e
        puts e.backtrace
        send_template(:error_message => e.message)
      end
    else
      send_template
    end
  end
  def search_by_file(file)
    raise "disabled" unless (settings.enable_upload_search)
    begin
      id, path = save_image(file.read)
      query = {:file => path}
      if (color_weight = get_color_weight)
        query[:color_weight] = color_weight
      end
      send_template(:query_url => "/upload/" + id,
                    :results => settings.otama.search(settings.result_max, query))
    rescue => e
      send_template(:error_message => e.message)
    end
  end
  def create_by_file(file)
    raise "disabled" unless (settings.enable_insert)
    begin
      blob = file.read
      id, path = save_image(blob)
      id = settings.otama.insert(:data => blob)
      settings.db[id] = path
      settings.otama.pull
      send_template(:error_message => "#{id} created.")
    rescue => e
      send_template(:error_message => e.message)
    end
  end
  def create_by_url(url)
    raise "disabled" unless (settings.enable_insert)
    begin
      blob = get_remote_content(url)
      id, path = save_image(blob)
      id = settings.otama.insert(:data => blob)
      settings.db[id] = path
      settings.otama.pull
      send_template(:error_message => "#{id} created.") 
    rescue => e
      send_template(:error_message => e.message)
    end
  end
  def send_template(options = {})
    @query_id = options[:query_id]
    @query_url = options[:query_url]
    @error_message = options[:error_message]
    @results = options[:results] || []
    @random_images = random_images || []
    @color_weight = get_color_weight
    erb :template
  end
  def image_filename(image_id)
    path = settings.db[image_id]
    if (File.exist?(path))
      path
    else
      File.join(ROOT, path)
    end
  end
  def upload_filename(file)
    File.join(settings.upload_dir, file)
  end
  def thumb_filename(image_id)
    File.join(settings.thumb_dir, sprintf("%s.jpg", image_id))
  end
  def get_color_weight
    if (color_weight_support?)
      params[:color_weight] ? params[:color_weight].to_f : settings.otama.get(:color_weight)
    else
      nil
    end
  end
  def save_image(blob)
    id = Otama.id(:data => blob)
    path = File.join(settings.upload_dir, id)
    File.open(path, "wb") do |f|
      f.write blob
    end
    [id, path]
  end
  def random_images(n = settings.result_max)
    results = []
    count = settings.db['COUNT'].to_i
    n.times do |i|
      image_id = settings.db[rand(count).to_i.to_s]
      if (image_id)
        results << image_id
      end
    end
    results
  end
  def get_remote_content(uri)
    content = nil
    timeout(settings.url_timeout) do
      begin
        meta = nil
        http_options = {
          :content_length_proc => Proc.new do |length |
            if (length)
              if (length > settings.max_upload_size)
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
