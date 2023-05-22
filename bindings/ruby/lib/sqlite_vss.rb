require "version"

module SqliteVss
  class Error < StandardError; end
  def self.vss_loadable_path
    File.expand_path('../vss0', __FILE__)
  end
  def self.vector_loadable_path
    File.expand_path('../vector0', __FILE__)
  end
  def self.load_vector(db)
    db.load_extension(self.vector_loadable_path)
  end
  def self.load_vss(db)
    db.load_extension(self.vss_loadable_path)
  end
  def self.load(db)
    self.load_vector(db)
    self.load_vss(db)
  end
end
