require 'rubygems'
require 'stringio'
require 'rest-client'
require 'json'

class OtamaClient
  def initialize(api_root)
    @api_root = api_root
  end

  def api_root
    @api_root
  end
  
  class DataIO < StringIO
    def path
      "temp"
    end
  end
  
  def insert(data)
    if (data.is_a?(Hash))
      if (data[:file])
        res = RestClient.post(api_root, 
                              :file => File.new(data[:file]))
      elsif (data[:data])
        res = RestClient.post(api_root, 
                              :file => DataIO.new(data[:data]))
      elsif (data[:string])
        res = RestClient.post(api_root, 
                              :string => data[:string])
      elsif (data[:id])
        res = RestClient.post(api_root,
                              :id => data[:id])
      else
        raise ArgumentError
      end
    elsif (arg.is_a?(String))
      res = RestClient.post(api_root, 
                            :file => File.new(data))
    else
      raise ArgumentError
    end
    JSON.parse(res)
  end
  def search(data)
    if (data.is_a?(Hash))
      if (data[:file])
        res = RestClient.post(File.join(api_root, "search"), 
                              :file => File.new(data[:file]))
      elsif (data[:data])
        res = RestClient.post(File.join(api_root, "search"), 
                              :file => DataIO.new(data[:data]))
      elsif (data[:string])
        res = RestClient.post(File.join(api_root, "search"), 
                              :string => data[:string])
      elsif (data[:id])
        res = RestClient.post(File.join(api_root, "search"),
                              :id => data[:id])
      else
        raise ArgumentError
      end
    elsif (arg.is_a?(String))
      res = RestClient.post(File.join(api_root, "search"), 
                            :file => File.new(data))
    else
      raise ArgumentError
    end
    JSON.parse(res)
  end
  def remove(otama_id)
    res = RestClient.delete(File.join(api_root, otama_id))
    JSON.parse(res)
  end
  def pull
    res = RestClient.post(File.join(api_root, "pull"), {})
    JSON.parse(res)
  end
end
