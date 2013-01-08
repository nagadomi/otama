# -*- coding: utf-8 -*-
require 'rubygems'
require 'filemagic'
require 'sinatra/base'
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

class ExampleWebApp < Sinatra::Base
  ROOT = File.expand_path(File.dirname(__FILE__))
  # otamaログレベル
  Otama.log_level = Otama::LOG_LEVEL_DEBUG
  # アップロード検索を許可するか
  ENABLE_UPLOAD_SEARCH = false
  # データベース登録を許可するか
  ENABLE_CREATE = false
  # 検索結果の列
  COLUMNS = 7
  # 検索結果の行
  ROWS = 6
  # 検索結果のサムネイルサイズ
  THUMB_SIZE = 96
  # 設定ファイル
  CONFIG_FILE = File.join(ROOT, 'config.yaml')
  # アップロードされたファイルを置くディレクトリ
  UPLOAD_DIR = File.join(ROOT, 'upload')
  # サムネイルを置くディレクトリ
  THUMB_DIR = File.join(ROOT, 'public/thumb')
  # IDとファイル名を対応付けるデータベース(make-database.rbで作成)
  DBM_FILE = File.join(ROOT, 'example.gdbm')
  # URLを指定して検索する場合の最大画像サイズ
  MAX_CONTENT_LENGTH = 10 * 1024 * 1024
  # URLを指定して検索する場合のタイムアウト
  CONTENT_TIMEOUT = 10
  
  RESULT_MAX = ROWS * COLUMNS
  
  configure do
    Dir.chdir(ROOT)
    
    set :db, GDBM::open(DBM_FILE, 0600, GDBM::NOLOCK|GDBM::READER|GDBM::WRITER)
    set :config, YAML.load_file(CONFIG_FILE)
    set :otama, Otama.open(settings.config)
    set :mime, FileMagic.new(FileMagic::MAGIC_MIME)

    settings.otama.pull
    
    trap(:TERM) do
      settings.otama.close
      exit
    end
    trap(:INT) do
      settings.otama.close
      exit
    end
    unless (File.directory?(UPLOAD_DIR))
      FileUtils.mkdir_p(UPLOAD_DIR)
    end
    unless (File.directory?(THUMB_DIR))
      FileUtils.mkdir_p(THUMB_DIR)
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
      thumb_path = thumb_filename(path)
      unless (File.exist?(thumb_path))
        img = Magick::Image.read(path).first
        img.resize_to_fit!(THUMB_SIZE, THUMB_SIZE)
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
      query = {:file => file}
      if (color_weight = get_color_weight)
        query[:color_weight] = color_weight
      end
      send_template(:query_id => image_id,
                    :results => settings.otama.search(RESULT_MAX, query))
    rescue => e
      puts e.backtrace
      send_template(:error_message => e.message)
     end
  end
  def search_by_url(url)
    raise "disabled" unless (ENABLE_UPLOAD_SEARCH)
    if (url =~ /^s?https?:\/\//)
      begin
        image = get_remote_content(url)
        query = {:data => image}
        if (color_weight = get_color_weight)        
          query[:color_weight] = color_weight
        end
        send_template(:query_url => url,
                      :results => settings.otama.search(RESULT_MAX, query))
      rescue => e
        puts e.backtrace
        send_template(:error_message => e.message)
      end
    else
      send_template
    end
  end
  def search_by_file(file)
    raise "disabled" unless (ENABLE_UPLOAD_SEARCH)
    begin
      id, path = save_image(file.read)
      query = {:file => path}
      if (color_weight = get_color_weight)
        query[:color_weight] = color_weight
      end
      send_template(:query_url => "/upload/" + id,
                    :results => settings.otama.search(RESULT_MAX, query))
    rescue => e
      send_template(:error_message => e.message)
    end
  end
  def create_by_file(file)
    raise "disabled" unless (ENABLE_CREATE)
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
    raise "disabled" unless (ENABLE_CREATE)
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
    File.join(UPLOAD_DIR, file)
  end
  def thumb_url(filename)
    File.join('/thumb', sprintf("%s.jpg", Otama.id(:file => filename)))
  end
  def thumb_filename(filename)
    File.join(THUMB_DIR, sprintf("%s.jpg", Otama.id(:file => filename)))
  end
  def color_weight_support?
    ["sim", "bovw2k_sboc", "bovw8k_sboc",
     "bovw2k_boc", "bovw8k_boc",
     "lmca_vlad_hsv", "lmca_vlad_colorcode"].include?(settings.config["driver"]["name"])
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
    path = File.join(UPLOAD_DIR, id)
    File.open(path, "wb") do |f|
      f.write blob
    end
    [id, path]
  end
  def random_images(n = RESULT_MAX)
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
