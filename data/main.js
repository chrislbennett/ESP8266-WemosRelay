(function() {
  angular.module('app',[]);

  angular.module('app').controller('main', ['$scope', '$http', '$log', function($scope, $http, $log) {

    $scope.relayOn = function() {
      $log.info('Relay On');
      $http.get('/relayon').then(function() {});
    };

    $scope.relayOff = function() {
      $log.info('Relay Off');
      $http.get('/relayoff').then(function() {});
    };

    $scope.saveDeviceName = function()
    {
      $http(
        {
          url: '/settings',
          method: 'POST',
          params: { deviceName: $scope.info.deviceName }
        }).then(function() {
          $log.info('Settings Saved');
        });
    };

    function activate()
    {
        $http.get('/info').then(function(data) {
          $scope.info = data.data;
          $log.info('Info Received');
        });
    }

    activate();
  }]);
})();
