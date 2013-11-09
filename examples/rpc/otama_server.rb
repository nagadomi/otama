require 'rubygems'
require 'sinatra'
require 'otama'
require 'json'

ROOT = File.expand_path(File.dirname(__FILE__))
Dir.chdir(ROOT);

CONFIG = 'config.yaml'
PULL_BEFORE_SEARCH = false
MAX_RESULT = 10
# Otama.log_level = Otama::LOG_LEVEL_DEBUG
Otama.set_log_file("./otama.log")

module OtamaInstance
  @@otama = nil
  def self.get
    if (@@otama)
      @@otama
    else
      @@otama = Otama.open(CONFIG)
      @@otama.create_database
      @@otama.pull
      @@otama
    end
  end
  def self.reset
    if (@@otama)
      @@otama.close
      @@otama = nil
    end
    get
  end
  def self.close
    if (@@otama)
      @@otama.close
      @@otama = nil
    end
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
def response_data(s, data = nil)
  if (data.nil?)
    {"status" => s }.to_json
  else
    {"status" => s, "data" => data }.to_json
  end
end

# insert
post '/' do
  syserror = false
  if (params[:file] && params[:file][:tempfile])
    begin
      otama_id = OtamaInstance.get.insert(:data => params[:file][:tempfile].read)
      status 200
      body response_data(200, :id => otama_id)
    rescue => e
      if (e.is_a?(Otama::SystemError))
        status 500
        body response_data(500)
        syserror = true
      else
        status 400
        body response_data(400)
      end
    end
  end
  if (syserror)
    OtamaInstance.reset
  end
end

# remove
delete '/:otama_id' do
  syserror = false
  begin
    OtamaInstance.get.remove(params[:otama_id]);
    body response_data(200)
  rescue => e
    if (e.is_a?(Otama::SystemError))
      status 500
      body response_data(500)
      syserror = true
    else
      status 400
      body response_data(400)
    end
  end
  if (syserror)
    OtamaInstance.reset
  end
end

# pull
post '/pull' do
  syserror = false
  begin
    OtamaInstance.get.pull
    status 200
    body response_data(200)
  rescue => e
    if (e.is_a?(Otama::SystemError))
      status 500
      body response_data(500)
      syserror = true
    else
      status 400
      body response_data(400)
    end
  end
  if (syserror)
    OtamaInstance.reset
  end
end

# search
search = lambda {
  syserror = false
  begin
    if (PULL_BEFORE_SEARCH)
      OtamaInstance.get.pull
    end
    results = nil
    if (params[:file])
      results = OtamaInstance.get.search(MAX_RESULT, :data => params[:file][:tempfile].read)
    elsif (params[:id])
      results = OtamaInstance.get.search(MAX_RESULT, :id => params[:id])
    elsif (params[:string])
      results = OtamaInstance.get.search(MAX_RESULT,
                                         :string => params[:string])
    end
    if (results)
      status 200
      body response_data(200, results.map{|rec| {"id" => rec.id, "value" => rec.value }})
    else
      status 400
      body response_data(400)
    end
  rescue => e
    if (e.is_a?(Otama::SystemError))
      status 500
      body response_data(500)
      syserror = true
    else
      status 400
      body response_data(400)
    end
  end
  if (syserror)
    OtamaInstance.reset
  end
}
get '/search', &search
post '/search', &search

