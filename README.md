# Render farm

```sh
# install openfaas inside kubernetes cluster
arkade install openfaas

# check that gateway is ready
kubectl rollout status -n openfaas deploy/gateway 

# forward gateway to be accessible outside cluster (create external gateway with Nodeport)
kubectl port-forward svc/gateway -n openfaas 8080:8080

# apply gateway timeouts patch to gateway deployment
kubectl patch deployment gateway -n openfaas --patch-file gateway-timeouts-patch.yaml

# apply faas-netes timeouts patch to gateway deployment
kubectl patch deployment gateway -n openfaas --patch-file faas-netes-timeouts-patch.yaml

# restart deployment to apply changes
kubectl rollout restart deployment gateway -n openfaas

# check that timeouts have been properly updated
kubectl get deployment gateway -n openfaas -o yaml | grep -A10 'env:' 

# build image
docker build -t zephyr75/render_farm:latest .

# push image to dockerhub
docker push zephyr75/render_farm:latest

# optional : run docker outside openfaas
docker run -p 8082:8080 -t zephyr75/render_farm:latest

# deploy dockerhub image to openfaas
faas-cli deploy -f stack.yaml

```
